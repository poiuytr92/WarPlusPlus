/***********************************************************
 * 
 * Projekt: Strategick� hra
 *    ��st: Abstraktn� datov� typy
 *   Autor: Karel Zikmund
 * 
 *   Popis: Typovan� memory pool
 * 
 ***********************************************************/

#ifndef __TYPED_MEMORY_POOL__HEADER_INCLUDED__
#define __TYPED_MEMORY_POOL__HEADER_INCLUDED__

#include <memory.h>
#include "Common\AfxDebugPlus\AfxDebugPlus.h"

//////////////////////////////////////////////////////////////////////
// �ablona typovan�ho memory poolu pro data "Element".
template <class Element>
class CTypedMemoryPool 
{
// Datov� typy
private:
	// hlavi�ka bloku dat
	struct SBlockHeader
	{
		// prvn� voln� polo�ka v bloku dat (NULL=blok je pln�)
		Element *pFirstFreeElement;
		// po�et pou�it�ch polo�ek v bloku dat
		DWORD dwElementCount;
		// ukazatel na dal�� blok dat
		SBlockHeader *pNextBlock;
	};
// Metody
public:
// Konstrukce a destrukce

	// konstruktor
	CTypedMemoryPool ( DWORD dwBlockSize );
	// destruktor
	~CTypedMemoryPool ();

	// zni�� nepot�ebnou pam�
	void FreeExtra ();

// Operace s prvky

	// alokuje nov� prvek
	//		v�jimky: CMemoryException
	Element *Allocate ();
	// dealokuje prvek
	void Free ( Element *pElement );
	// dealokuje v�echny prvky
	void FreeAll ();

// Data
private:
	// ukazatel na prvn� blok dat
	SBlockHeader *m_pFirstBlock;
	// ukazatel na prvn� voln� blok dat (nezahrnuje �pln� pr�zdn� bloky)
	SBlockHeader *m_pFirstFreeBlock;
	// ukazatel na pr�zdn� blok
	SBlockHeader *m_pEmptyBlock;
	// po�et datov�ch polo�ek v bloku
	DWORD m_dwBlockSize;
};

//////////////////////////////////////////////////////////////////////
// Konstrukce a destrukce
//////////////////////////////////////////////////////////////////////

// konstruktor
template <class Element> 
inline CTypedMemoryPool<Element>::CTypedMemoryPool ( DWORD dwBlockSize ) 
{
	ASSERT ( dwBlockSize > 0 );
	ASSERT ( sizeof ( Element ) >= sizeof ( Element * ) );

	// inicializuje data memory poolu
	m_pFirstBlock = NULL;
	m_pFirstFreeBlock = NULL;
	m_pEmptyBlock = NULL;

	// inicializuje velikost bloku
	m_dwBlockSize = dwBlockSize;
}

// destruktor
template <class Element> 
CTypedMemoryPool<Element>::~CTypedMemoryPool () 
{
	// uvoln� nepot�ebnou pam� (tj. pr�zdn� blok)
	FreeExtra ();
	ASSERT ( m_pEmptyBlock == NULL );

#ifdef _DEBUG
// zkontroluje, je-li memory pool pr�zdn�

// pomocn� prom�nn�

	// ukazatel na blok
	SBlockHeader *pBlock = m_pFirstBlock;
	// po�et alokovan�ch prvk�
	DWORD dwElementCount = 0;
	// p��znak nalezen� prvn�ho voln�ho bloku
	BOOL bFirstFreeBlockFound = ( m_pFirstFreeBlock == NULL );

// zkontroluje, je-li memory pool pr�zdn�

	// spo��t� po�et alokovan�ch prvk�
	while ( pBlock != NULL )
	{
		ASSERT ( ( pBlock->dwElementCount > 0 ) && ( pBlock->dwElementCount <= m_dwBlockSize ) );

	// zkontroluje spoj�k voln�ch prvk� bloku

		// ukazatel na voln� prvek bloku
		Element *pElement = pBlock->pFirstFreeElement;
		// projede spoj�k voln�ch prvk� bloku
		for ( DWORD dwIndex = m_dwBlockSize - pBlock->dwElementCount; dwIndex-- > 0; )
		{
			// zjist�, je-li to skute�n� prvek bloku
			if ( ( pElement == NULL ) || ( pElement < (Element *)(pBlock + 1) ) || 
				( pElement >= ((Element *)(pBlock + 1)) + m_dwBlockSize ) || 
				( ((char *)pElement - (char *)(pBlock + 1)) % sizeof ( Element ) != 0 ) )
			{	// spoj�k pokra�uje na chybn� adrese
				ASSERT ( FALSE );
			}
			// vezme dal�� prvek spoj�ku
			pElement = *(Element **)pElement;
		}
		// zkontroluje ukon�en� spoj�ku
		if ( pElement != NULL )
		{	// spoj�k nen� ukon�en
			ASSERT ( FALSE );
		}

	// zpracuje nepr�zdn� blok prvk�

		// zjist�, jedn�-li se o prvn� voln� blok
		if ( pBlock == m_pFirstFreeBlock )
		{	// jedn� se o prvn� voln� blok
			bFirstFreeBlockFound = TRUE;
		}
		// p�i�te alokovan� prvky bloku
		dwElementCount += pBlock->dwElementCount;
		// p�ejde na dal�� blok
		pBlock = pBlock->pNextBlock;
	}

	// zkontroluje, je-li ukazatel na prvn� voln� blok platn�
	ASSERT ( bFirstFreeBlockFound );

	// zjist�, jsou-li alokov�ny n�jak� prvky
	if ( dwElementCount != 0 )
	{	// jsou alokov�ny n�jak� prvky
		TRACE2 ( "Memory leaks detected in memory pool: %d elements allocated of size %d\n", dwElementCount, sizeof ( Element ) );
		// index dumpovan�ho prvku
		DWORD dwElementIndex = 0;

	// vydumpuje alokovan� prvky

		// projede spoj�k blok�
		for ( pBlock = m_pFirstBlock; pBlock != NULL; pBlock = pBlock->pNextBlock )
		{
			// ukazatel na prvek bloku
			Element *pElement = (Element *)(pBlock + 1);
			// projede prvky bloku
			for ( DWORD dwIndex = m_dwBlockSize; dwIndex-- > 0; pElement++ )
			{	// zjist�, je-li prvek bloku voln�
				// p��znak, je-li prvek voln�
				BOOL bIsFree = FALSE;
				// projede spoj�k voln�ch prvk�
				for ( Element *pFreeElement = pBlock->pFirstFreeElement; pFreeElement != NULL; 
					pFreeElement = *(Element **)pFreeElement )
				{
					// zjist�, jedn�-li se o hledan� prvek bloku
					if ( pFreeElement == pElement )
					{	// jedn� se o hledan� prvek bloku
						bIsFree = TRUE;
						break;
					}
				}
				// zjist�, je-li prvek bloku voln�
				if ( !bIsFree )
				{	// prvek bloku je alokovan�
					// vydumpuje prvek bloku
					TRACE1_NEXT ( "   Dumping element #%d of memory pool: ", ++dwElementIndex );
					for ( int i = 0; i < sizeof ( Element ); i++ )
					{
						TRACE1_NEXT ( " %02X", (int)((unsigned char*)pElement)[i] );
					}
					TRACE0_NEXT ( "\n" );
				}
			}
		}
		ASSERT ( FALSE );
	}
#endif //_DEBUG
}

template <class Element> 
// zni�� nepot�ebnou pam�
void CTypedMemoryPool<Element>::FreeExtra () 
{
	// zjist�, je-li alokov�n pr�zdn� blok
	if ( m_pEmptyBlock != NULL )
	{	// je alokov�n pr�zdn� blok
		ASSERT ( m_pEmptyBlock->dwElementCount == 0 );
		ASSERT ( m_pEmptyBlock->pNextBlock == NULL );
#ifdef _DEBUG
	// zkontroluje, je-li pole prvk� skute�n� pr�zdn�

		// ukazatel na voln� prvek bloku
		Element *pElement = m_pEmptyBlock->pFirstFreeElement;
		// projede spoj�k voln�ch prvk� bloku
		for ( DWORD dwIndex = m_dwBlockSize; dwIndex-- > 0; )
		{
			// zjist�, je-li to skute�n� prvek bloku
			if ( ( pElement == NULL ) || ( pElement < (Element *)(m_pEmptyBlock + 1) ) || 
				( pElement >= ((Element *)(m_pEmptyBlock + 1)) + m_dwBlockSize ) || 
				( ((char *)pElement - (char *)(m_pEmptyBlock + 1)) % sizeof ( Element ) != 0 ) )
			{	// spoj�k pokra�uje na chybn� adrese
				ASSERT ( FALSE );
			}
			// vezme dal�� prvek spoj�ku
			pElement = *(Element **)pElement;
		}
		// zkontroluje ukon�en� spoj�ku
		if ( pElement != NULL )
		{	// spoj�k nen� ukon�en
			ASSERT ( FALSE );
		}
#endif //_DEBUG

		// zni�� alokovanou pam�
		delete [](BYTE *)m_pEmptyBlock;
		m_pEmptyBlock = NULL;
	}
}

//////////////////////////////////////////////////////////////////////
// Operace s prvky
//////////////////////////////////////////////////////////////////////

// alokuje nov� prvek
//		v�jimky: CMemoryException
template <class Element> 
Element *CTypedMemoryPool<Element>::Allocate () 
{
	// zjist�, je-li n�jak� blok voln�
	if ( m_pFirstFreeBlock == NULL )
	{	// ��dn� blok nen� voln�
		// zjist�, je-li n�jak� blok �pln� pr�zdn�
		if ( m_pEmptyBlock == NULL )
		{	// ��dn� blok nen� �pln� pr�zdn�
			// alokuje nov� blok
			m_pFirstFreeBlock = (SBlockHeader *)new BYTE[sizeof ( SBlockHeader ) + 
				m_dwBlockSize * sizeof ( Element )];
			// inicializuje nov� alokovan� blok
			m_pFirstFreeBlock->pNextBlock = m_pFirstBlock;
			m_pFirstBlock = m_pFirstFreeBlock;
			m_pFirstFreeBlock->dwElementCount = 0;

		// pomocn� prom�nn� pro pr�jezd polem pr�zdn�ch prvk�

			// ukazatel na prvek v poli prvk� bloku
			Element *pElement = (Element *)(m_pFirstFreeBlock + 1);
			// inicializuje ukazatel na prvn� voln� prvek
			m_pFirstFreeBlock->pFirstFreeElement = pElement;

			// inicializuje seznam nepou�it�ch prvk� nov� alokovan�ho bloku
			for ( DWORD dwIndex = m_dwBlockSize; dwIndex-- > 0; )
			{
				*(Element **)pElement = pElement + 1;
				pElement++;
			}
			// ukon�� inicializaci seznamu
			*(Element **)(pElement - 1) = NULL;

			// m�me p�ipraven� nov� alokovan� voln� blok
		}
		else
		{	// m�me �pln� pr�zdn� blok
			ASSERT ( m_pEmptyBlock->pNextBlock == NULL );
			// zm�n� pr�zdn� blok na voln� blok
			m_pEmptyBlock->pNextBlock = m_pFirstBlock;
			m_pFirstBlock = m_pFirstFreeBlock = m_pEmptyBlock;
			m_pEmptyBlock = NULL;
		}
	}
	// m�me voln� blok
	ASSERT ( m_pFirstFreeBlock->dwElementCount < m_dwBlockSize );

	// schov� si voln� prvek
	Element *pElement = m_pFirstFreeBlock->pFirstFreeElement;
	// aktualizuje po�et pou�it�ch prvk�
	++m_pFirstFreeBlock->dwElementCount;
	// aktualizuje prvn� voln� prvek
	if ( ( m_pFirstFreeBlock->pFirstFreeElement = *(Element **)pElement ) == NULL )
	{	// jedn� se o posledn� voln� prvek bloku
		// pokus� se naj�t voln� blok
		do
		{
			// p�ejde na dal�� blok
			m_pFirstFreeBlock = m_pFirstFreeBlock->pNextBlock;
			// zjist�, je-li blok voln� (a nen�-li to ji� posledn� blok)
		} while ( ( m_pFirstFreeBlock != NULL ) && 
			( m_pFirstFreeBlock->dwElementCount == m_dwBlockSize ) );
	}

	// vr�t� prvek
	return pElement;
}

// dealokuje prvek
template <class Element> 
void CTypedMemoryPool<Element>::Free ( Element *pElement ) 
{
	ASSERT ( pElement != NULL );

// pomocn� prom�nn�

	// ukazatel na odkaz na blok
	SBlockHeader **pBlock = &m_pFirstBlock;

	// najde blok, ve kter�m je prvek alokov�n
	while ( *pBlock != NULL )
	{
		// zjist�, je-li prvek z tohoto bloku
		if ( ( (Element *)(*pBlock) < pElement ) && 
			( ((Element *)((*pBlock) + 1)) + m_dwBlockSize > pElement ) )
		{	// prvek je z tohoto bloku
			// zkontroluje, dealokuje-li se platn� ukazatel
			ASSERT ( (*pBlock)->dwElementCount > 0 );
			ASSERT ( (BYTE *)((*pBlock) + 1) <= (BYTE *)pElement );
			ASSERT ( ( (BYTE *)pElement - (BYTE *)((*pBlock) + 1) ) % sizeof ( Element ) == 0 );
#ifdef _DEBUG
		// zkontroluje, nen�-li dealokovan� prvek voln�

			// pomocn� prom�nn� pro pr�jezd spoj�ku voln�ch prvk�
			Element *pFreeElement = (*pBlock)->pFirstFreeElement;

			// projede spoj�k voln�ch prvk�
			while ( pFreeElement != NULL )
			{
				// zjist�, je-li to dealokovan� prvek
				if ( pFreeElement == pElement )
				{	// dealokovan� prvek je voln�
					TRACE1 ( "Error deallocating element in the memory pool - element already deallocated! (element size %d)\n", sizeof ( Element ) );
					ASSERT ( FALSE );
					return;
				}
				pFreeElement = *(Element **)pFreeElement;
			}

			// inicializuje dealokovan� prvek
			memset ( (void *)pElement, 0xfd, sizeof ( Element ) );
#endif //_DEBUG
			// p�id� prvek do seznamu voln�ch prvk�
			*(Element **)pElement = (*pBlock)->pFirstFreeElement;
			(*pBlock)->pFirstFreeElement = pElement;

			// aktualizuje po�et pou�it�ch prvk� bloku
			if ( --(*pBlock)->dwElementCount == 0 )
			{	// blok je pr�zdn�
				// z�sk� ukazatel na pr�zdn� blok
				SBlockHeader *pEmptyBlock = *pBlock;

				// zjist�, je-li to tak� prvn� voln� blok
				if ( pEmptyBlock == m_pFirstFreeBlock )
				{	// jedn� se o prvn� voln� blok
					// pokus� se naj�t voln� blok
					do
					{
						// p�ejde na dal�� blok
						m_pFirstFreeBlock = m_pFirstFreeBlock->pNextBlock;
						// zjist�, je-li blok voln� (a nen�-li to ji� posledn� blok)
					} while ( ( m_pFirstFreeBlock != NULL ) && 
						( m_pFirstFreeBlock->dwElementCount == m_dwBlockSize ) );
				}

				// vyjme blok ze seznamu pou��van�ch blok�
				*pBlock = pEmptyBlock->pNextBlock;
				// aktualizuje ukazatel na dal�� blok
				pEmptyBlock->pNextBlock = NULL;

				// zjist�, je-li to prvn� pr�zdn� blok
				if ( m_pEmptyBlock == NULL )
				{	// jedn� se o prvn� pr�zdn� blok
					m_pEmptyBlock = pEmptyBlock;
				}
				else
				{	// jedn� se o dal�� pr�zdn� blok
					// zni�� pr�zdn� blok
					delete [] (BYTE *)pEmptyBlock;
				}
			}
			// ukon�� dealokaci prvku
			return;
		}
		// p�ejde na dal�� blok
		pBlock = &(*pBlock)->pNextBlock;
	}

	// nena�el alokovan� prvek
	TRACE1 ( "Error deallocating element in the memory pool - element was not allocated here! (element size %d)\n", sizeof ( Element ) );
	ASSERT ( FALSE );
}

// dealokuje v�echny prvky
template <class Element> 
void CTypedMemoryPool<Element>::FreeAll () 
{
	// ukazatel na ni�en� blok
	struct SBlockHeader *pBlock = m_pFirstBlock;

	// zjist�, jedn�-li se o prvn� pr�zdn� blok
	if ( ( pBlock != NULL ) && ( m_pEmptyBlock == NULL ) )
	{	// jedn� se o prvn� pr�zdn� blok
		// aktualizuje ukazatel na pr�zdn� blok
		m_pEmptyBlock = pBlock;

		// aktualizuje ukazatel na dal�� ni�en� blok
		pBlock = pBlock->pNextBlock;

		// aktualizuje pr�zdn� blok
		m_pEmptyBlock->pNextBlock = NULL;
		m_pEmptyBlock->dwElementCount = 0;

		// ukazatel na prvek v poli prvk� bloku
		Element *pElement = (Element *)(m_pEmptyBlock + 1);
		// inicializuje ukazatel na prvn� voln� prvek
		m_pEmptyBlock->pFirstFreeElement = pElement;

		// inicializuje seznam nepou�it�ch prvk� nov� alokovan�ho bloku
		for ( DWORD dwIndex = m_dwBlockSize; dwIndex-- > 0; )
		{
			*(Element **)pElement = pElement + 1;
			pElement++;
		}
		// ukon�� inicializaci seznamu
		*(Element **)(pElement - 1) = NULL;
	}

	// aktualizuje ukazatel na prvn� pr�zdn� blok
	m_pFirstFreeBlock = NULL;
	// aktualizuje ukazatel na prvn� blok
	m_pFirstBlock = NULL;

	// zni�� v�echny bloky
	while ( pBlock != NULL )
	{
		// ukazatel na dal�� ni�en� blok
		struct SBlockHeader *pNextBlock = pBlock->pNextBlock;

		// zni�� blok
		delete [] (BYTE *)pBlock;

		// aktualizuje ukazatel na dal�� ni�en� blok
		pBlock = pNextBlock;
	}
}

#endif //__TYPED_MEMORY_POOL__HEADER_INCLUDED__
