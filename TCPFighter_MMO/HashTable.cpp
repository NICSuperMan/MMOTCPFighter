#include <windows.h>
#include "LinkedList.h"
#include "HashTable.h"

// Returns the starting address of a STATIC_HASH_METADATA structure
// when the address of a member variable link (of type LINKED_NODE*),a member variable in a STATIC_HASH_METADATA structure,is specified as an argument pLink.
__forceinline static STATIC_HASH_METADATA* lnToMetaData(LINKED_NODE* pLink)
{
    STATIC_HASH_METADATA* retAddr;
    retAddr = (STATIC_HASH_METADATA*)((char*)pLink - offsetof(STATIC_HASH_METADATA, link));
    return retAddr;
}

// Returns the starting address of a BUCKET_DESCRIPTOR structure
// when the address of a member variable link (of type LINKED_NODE*),a member variable in a BUCKET_DESCRIPTOR structure, is specified as an argument pLink.
__forceinline static BUCKET_DESCRIPTOR* lnToBucketDesc(LINKED_NODE* pLink)
{
    BUCKET_DESCRIPTOR* retAddr;
    retAddr = (BUCKET_DESCRIPTOR*)((char*)pLink - offsetof(BUCKET_DESCRIPTOR, link));
    return retAddr;
}

// Returns the starting address of a Bucket(which is inserted by user of HashTable)
// For this function to work properly, users using hashtables must declare a STATIC_HASH_METADATA structure in the structure they are inserting.
// The difference between the start address of the structure inserted by the user and the start address of STATIC_HASH_METADATA is stored in dwHashMetaDataOffset_
// The second parameter, pHashTable, exists because of the member variable dwHashMetaDataOffset_.
__forceinline static void* MetaDataToBucket(const STATIC_HASH_METADATA* pMetaData, const CStaticHashTable* pHashTable)
{
    void* retAddr;
    retAddr = ((char*)pMetaData - pHashTable->dwHashMetaDataOffset_);
    return retAddr;
}

// Returns the starting address of a STATIC_HASH_METADATA structure
// Provides the opposite functionality of the MetaDataToBucket function.
// Similarly, For this function to work properly, users using hashtables must declare a STATIC_HASH_METADATA structure in the structure they are inserting. 
__forceinline STATIC_HASH_METADATA* BucketToMetaData(const void* pBucket, const CStaticHashTable* pHashTable)
{
    STATIC_HASH_METADATA* retAddr;
    retAddr = (STATIC_HASH_METADATA*)((char*)pBucket + pHashTable->dwHashMetaDataOffset_);
    return retAddr;
}

BOOL CStaticHashTable::Initialize(DWORD dwMaxBucketNum, DWORD dwKeySize, HASH_FUNC keyFunc, KEY_ASSIGN_FUNC keyAssignFunc, KEY_COMPARE_FUNC keyCompareFunc, DWORD dwHashMetaDataOffset)
{
    dwMaxBucketNum_ = dwMaxBucketNum;
    dwKeySize_ = dwKeySize;
    pBucketDescriptorTable_ = new BUCKET_DESCRIPTOR[dwMaxBucketNum];
    hashFunc_ = keyFunc;
    keyAssignFunc_ = keyAssignFunc;
    keyCompareFunc_ = keyCompareFunc;
    dwHashMetaDataOffset_ = dwHashMetaDataOffset;
    return true;
}

DWORD CStaticHashTable::Find(void** ppOutItemList, DWORD dwMaxItemNum, const void* pKeyData, DWORD dwKeySize)
{
    DWORD dwFindedItemNum;
    BUCKET_DESCRIPTOR* pBucketDesc;
    DWORD dwIndex;
    STATIC_HASH_METADATA* pCurMetaData;
    LINKED_NODE* pCurLinkedNode;

    if (dwKeySize != dwKeySize_)
    {
        __debugbreak();
        goto lb_return;
    }
    dwFindedItemNum = 0;
    dwIndex = hashFunc_(pKeyData);
    pBucketDesc = pBucketDescriptorTable_ + dwIndex;
    pCurLinkedNode = pBucketDesc->pBucketLinkHead;
    while (pCurLinkedNode)
    {
        pCurMetaData = lnToMetaData(pCurLinkedNode);
        if (!dwMaxItemNum)
        {
            goto lb_return;
        }

        if (!keyCompareFunc_(pCurMetaData->pKey, pKeyData))
        {
			// the key value to find is different from the current bucket's key value
            goto lb_next;
        }
        --dwMaxItemNum;
        ppOutItemList[dwFindedItemNum] = MetaDataToBucket(pCurMetaData, this);
        ++dwFindedItemNum;
    lb_next:
        pCurLinkedNode = pCurLinkedNode->pNext;
    }
lb_return:
    return dwFindedItemNum;
}

void CStaticHashTable::Insert(const void* pBucket, const void* pKey, DWORD dwKeySize)
{
    DWORD dwIndex;
    BUCKET_DESCRIPTOR* pBucketDesc;
    STATIC_HASH_METADATA* pHashMetaData;
    if (dwKeySize > dwKeySize_)
    {
        __debugbreak();
        return;
    }

	dwIndex = hashFunc_(pKey);
	pBucketDesc = pBucketDescriptorTable_ + dwIndex;
    pHashMetaData = BucketToMetaData(pBucket, this);
    pHashMetaData->pBucketDesc = pBucketDesc;
    keyAssignFunc_(&(pHashMetaData->pKey), pKey);
    LinkToLinkedListLast(&pBucketDesc->pBucketLinkHead, &pBucketDesc->pBucketLinkTail, &(pHashMetaData->link));
    if (pBucketDesc->dwLinkNodeNum == 0)
    {
        // Add BUCKET_DESCRIPTOR to the linked list for traversal
        LinkToLinkedListLast(&pBucketDescHead_, &pBucketDescTail_, &(pBucketDesc->link));
    }
    ++(pBucketDesc->dwLinkNodeNum);
    ++dwItemNum_;
}

void CStaticHashTable::Delete(const void* pBucket)
{
    STATIC_HASH_METADATA* pHashMetaData;
    BUCKET_DESCRIPTOR* pBucketDesc;

    pHashMetaData = BucketToMetaData(pBucket, this);
    pBucketDesc = pHashMetaData->pBucketDesc;
    UnLinkFromLinkedList(&(pBucketDesc->pBucketLinkHead), &(pBucketDesc->pBucketLinkTail), &pHashMetaData->link);
    --pBucketDesc->dwLinkNodeNum;
    if (pBucketDesc->dwLinkNodeNum == 0)
    {
        // remove BUCKET_DESCRIPTOR to the linked list for traversal
        UnLinkFromLinkedList(&pBucketDescHead_, &pBucketDescTail_, &(pBucketDesc->link));
    }
    --dwItemNum_;
}

DWORD CStaticHashTable::GetMaxBucketNum() const
{
    return dwMaxBucketNum_;
}

DWORD CStaticHashTable::GetAllItems(void** ppOutBucketList, DWORD dwMaxItemNum, BOOL* pbOutInsufficient) const
{
    STATIC_HASH_METADATA* pHashMetaData;
    LINKED_NODE* pCurLink;
    DWORD dwItemNum;

    if (dwMaxItemNum > dwItemNum_)
    {
        __debugbreak();
    }

    *pbOutInsufficient = FALSE;
    dwItemNum = 0;

    for (DWORD i = 0; i < dwMaxBucketNum_; ++i)
    {
        pCurLink = pBucketDescriptorTable_[i].pBucketLinkHead;
        while (pCurLink)
        {
            pHashMetaData = lnToMetaData(pCurLink);
            if (dwItemNum >= dwMaxItemNum)
            {
                *pbOutInsufficient = TRUE;
                goto lb_return;
            }
            ppOutBucketList[dwItemNum] = MetaDataToBucket(pHashMetaData, this);
            ++dwItemNum;
            pCurLink = pCurLink->pNext;
        }
    }
    lb_return:
    return dwItemNum;
}


DWORD CStaticHashTable::GetItemNum() const
{
    return dwItemNum_;
}

void* CStaticHashTable::GetFirst()
{
    void* retAddr;
    BUCKET_DESCRIPTOR* pFirstBucketDesc;
    STATIC_HASH_METADATA* pFirstHashMetaData;

    retAddr = nullptr;
    // HashTable is empty
    if (!pBucketDescHead_)
    {
#ifdef _DEBUG
        if (dwItemNum_ > 0)
        {
            __debugbreak();
        }
#endif
        goto lb_return;
    }
    pFirstBucketDesc = lnToBucketDesc(pBucketDescHead_);
    pFirstHashMetaData = lnToMetaData(pFirstBucketDesc->pBucketLinkHead);
    retAddr = MetaDataToBucket(pFirstHashMetaData, this);

lb_return:
    return retAddr;
}

void* CStaticHashTable::GetNext(const void* pBucket)
{
    void* retAddr;
    STATIC_HASH_METADATA* pHashMetaData;
    LINKED_NODE* pNextBucketLink;
    LINKED_NODE* pNextBucketDescLink;
    BUCKET_DESCRIPTOR* pBucketDesc;
    if (!pBucket)
    {
        __debugbreak();
    }

    retAddr = nullptr;
    pHashMetaData = BucketToMetaData(pBucket, this);
    pNextBucketLink = pHashMetaData->link.pNext;

    if (!pNextBucketLink)
    {
	    // pBucket is the last element in the linked list for the hash value of the currently specified key.
        // Back to current BUCKET_DESCRIPTOR
        pBucketDesc = pHashMetaData->pBucketDesc;
        pNextBucketDescLink = pBucketDesc->link.pNext;
        if (!pNextBucketDescLink)
        {
            // current BUCKET_DESCRIPTOR(pBucketDesc) is last bucket descriptor in the linked list
            goto lb_return;
        }

        // pBucketDesc is updated and must be pointing to some bucket
        pBucketDesc = lnToBucketDesc(pNextBucketDescLink);
        pNextBucketLink = pBucketDesc->pBucketLinkHead;
    }

#ifdef _DEBUG
    // pBucketDesc must point to some bucket (probably the linked list is Cause of problem)
    if (!pNextBucketLink)
    {
        __debugbreak();
    }
#endif
    pHashMetaData = lnToMetaData(pNextBucketLink);
    retAddr = MetaDataToBucket(pHashMetaData, this);
lb_return:
    return retAddr;
}

CStaticHashTable::CStaticHashTable()
{
}

CStaticHashTable::~CStaticHashTable()
{
    delete[] pBucketDescriptorTable_;
    pBucketDescriptorTable_ = nullptr;
}
