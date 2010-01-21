#include <ir/index_manager/index/Posting.h>
#include <ir/index_manager/index/OutputDescriptor.h>
#include <ir/index_manager/index/InputDescriptor.h>
#include <ir/index_manager/store/IndexOutput.h>
#include <ir/index_manager/store/IndexInput.h>

#include <math.h>


using namespace std;

using namespace izenelib::ir::indexmanager;

int32_t InMemoryPosting::UPTIGHT_ALLOC_CHUNKSIZE = 8;
int32_t InMemoryPosting::UPTIGHT_ALLOC_MEMSIZE = 40000;

Posting::Posting()
    :pDocFilter_(0)
{}

Posting::~Posting()
{}


//////////////////////////////////////////////////////////////////////////
///CompressedPostingList
bool CompressedPostingList::addPosting(uint32_t posting32)
{
    if (pTailChunk_ == NULL)
        return false;
    int32_t left = pTailChunk_->size - nPosInCurChunk_;

    if (left < 7)///at least 4 free space
    {
        nTotalUnused_ += left;///Unused size
        pTailChunk_->size = nPosInCurChunk_;///the real size
        return false;
    }

    uint32_t ui = posting32;
    while ((ui & ~0x7F) != 0)
    {
        pTailChunk_->data[nPosInCurChunk_++] = ((uint8_t)((ui & 0x7f) | 0x80));
        ui >>= 7;
    }
    pTailChunk_->data[nPosInCurChunk_++] = (uint8_t)ui;
    return true;
}

bool CompressedPostingList::addPosting(uint64_t posting64)
{
    if (pTailChunk_ == NULL)
        return false;
    int32_t left = pTailChunk_->size - nPosInCurChunk_;
    if (left < 11)///at least 8 free space
    {
        nTotalUnused_ += left;///Unused size
        pTailChunk_->size = nPosInCurChunk_;///the real size
        return false;
    }

    uint64_t ui = posting64;
    while ((ui & ~0x7F) != 0)
    {
        pTailChunk_->data[nPosInCurChunk_++] = ((uint8_t)((ui & 0x7f) | 0x80));
        ui >>= 7;
    }
    pTailChunk_->data[nPosInCurChunk_++] = ((uint8_t)ui);

    return true;
}
int32_t CompressedPostingList::decodePosting32(uint8_t*& posting)
{
    uint8_t b = *posting++;
    int32_t i = b & 0x7F;
    for (int32_t shift = 7; (b & 0x80) != 0; shift += 7)
    {
        b = *posting++;
        i |= (b & 0x7FL) << shift;
    }
    return i;
}

int64_t CompressedPostingList::decodePosting64(uint8_t*& posting)
{
    uint8_t b = *posting++;
    int64_t i = b & 0x7F;
    for (int32_t shift = 7; (b & 0x80) != 0; shift += 7)
    {
        b = *posting++;
        i |= (b & 0x7FLL) << shift;
    }
    return i;
}

void CompressedPostingList::encodePosting32(uint8_t*& posting,int32_t val)
{
    uint32_t ui = val;
    while ((ui & ~0x7F) != 0)
    {
        *posting++ = ((uint8_t)((ui & 0x7f) | 0x80));
        ui >>= 7;
    }
    *posting++ = (uint8_t)ui;
}
void CompressedPostingList::encodePosting64(uint8_t*& posting,int64_t val)
{
    uint64_t ui = val;
    while ((ui & ~0x7F) != 0)
    {
        *posting++ = ((uint8_t)((ui & 0x7f) | 0x80));
        ui >>= 7;
    }
    *posting++ = ((uint8_t)ui);
}

void CompressedPostingList::truncTailChunk()
{
    nTotalUnused_ += pTailChunk_->size - nPosInCurChunk_;
    pTailChunk_->size = nPosInCurChunk_;
}

void CompressedPostingList::addChunk(PostingChunk* pChunk)
{
    if (pTailChunk_)
        pTailChunk_->next = pChunk;
    pTailChunk_ = pChunk;
    if (!pHeadChunk_)
        pHeadChunk_ = pTailChunk_;
    nTotalSize_ += pChunk->size;

    nPosInCurChunk_ = 0;
}
int32_t CompressedPostingList::getRealSize()
{
    return nTotalSize_ - nTotalUnused_;
}

void CompressedPostingList::reset()
{
    pHeadChunk_ = pTailChunk_ = NULL;
    nTotalSize_ = nPosInCurChunk_ = nTotalUnused_ = 0;
}

//////////////////////////////////////////////////////////////////////////
///InMemoryPosting

InMemoryPosting::InMemoryPosting(MemCache* pCache)
        :pMemCache_(pCache)
        ,nDF_(0)
        ,nLastDocID_(BAD_DOCID)
        ,nLastLoc_(BAD_DOCID)
        ,nCurTermFreq_(0)
        ,nCTF_(0)
        ,pDS_(NULL)
{
    pDocFreqList_ = new CompressedPostingList();
    pLocList_  = new CompressedPostingList();

    int32_t newSize = getNextChunkSize(pDocFreqList_->nTotalSize_);
    pDocFreqList_->addChunk(newChunk(newSize));
    newSize = getNextChunkSize(pLocList_->nTotalSize_);
    pLocList_->addChunk(newChunk(newSize));
}

InMemoryPosting::InMemoryPosting()
        :pMemCache_(NULL)
        ,nDF_(0)
        ,nLastDocID_(BAD_DOCID)
        ,nLastLoc_(BAD_DOCID)
        ,nCurTermFreq_(0)
        ,nCTF_(0)
        ,pDS_(NULL)
        ,pDocFreqList_(NULL)
        ,pLocList_(NULL)
{
}

InMemoryPosting::~InMemoryPosting()
{
    if (pDocFreqList_)
    {
        delete pDocFreqList_;
        pDocFreqList_ = NULL;
    }
    if (pLocList_)
    {
        delete pLocList_;
        pLocList_ = NULL;
    }
    pMemCache_ = NULL;

    if (pDS_)
    {
        delete pDS_;
        pDS_ = NULL;
    }
}

PostingChunk* InMemoryPosting::newChunk(int32_t chunkSize)
{
    uint8_t* begin = pMemCache_->getMem(chunkSize);
    ///allocate memory failed,decrease chunk size
    if (!begin)
    {
        ///into UPTIGHT state
        begin = pMemCache_->getMem(UPTIGHT_ALLOC_CHUNKSIZE);
        ///allocation failed again, grow memory cache.
        if (!begin)
        {
            begin  = pMemCache_->grow(UPTIGHT_ALLOC_MEMSIZE)->getMem(UPTIGHT_ALLOC_CHUNKSIZE);
            if (!begin)
            {
                SF1V5_THROW(ERROR_OUTOFMEM,"InMemoryPosting:newChunk() : Allocate memory failed.");
            }
        }
        chunkSize = UPTIGHT_ALLOC_CHUNKSIZE;
    }

    PostingChunk* pChunk = (PostingChunk*)begin;
    pChunk->size = (int32_t)(POW_TABLE[chunkSize] - sizeof(PostingChunk*) - sizeof(int32_t));
    pChunk->next = NULL;
    return pChunk;
}

fileoffset_t InMemoryPosting::write(OutputDescriptor* pOutputDescriptor)
{
    ///flush last document
    flushLastDoc(true);

    IndexOutput* pDOutput = pOutputDescriptor->getDPostingOutput();

    ///write chunk data
    writeDPosting(pDOutput);

    ///save the offset of posting descriptor
    fileoffset_t poffset = pDOutput->getFilePointer();

    IndexOutput* ptiOutput = pOutputDescriptor->getPPostingOutput();
    ///write position posting data
    fileoffset_t positionPointer = writePPosting(ptiOutput);
    ///write descriptors
    writeDescriptor(pDOutput,positionPointer);

    return poffset;
}

void InMemoryPosting::reset()
{
    pDocFreqList_->reset();
    pLocList_->reset();

    nCTF_ = 0;
    nLastDocID_ = BAD_DOCID;
    nLastLoc_ = 0;
    nDF_ = 0;
    nCurTermFreq_ = 0;

    if (pDS_)
    {
        delete pDS_;
        pDS_ = NULL;
    }
}

Posting* InMemoryPosting::clone()
{
    InMemoryPosting* pClone = new InMemoryPosting();
    if (pDocFreqList_)
    {
        pClone->pDocFreqList_ = new CompressedPostingList(*pDocFreqList_);
    }
    if (pLocList_)
    {
        pClone->pLocList_ = new CompressedPostingList(*pLocList_);
    }
    pClone->nCTF_ = nCTF_;
    pClone->nDF_ = nDF_;
    pClone->nCurTermFreq_ = nCurTermFreq_;
    pClone->nLastDocID_ = nLastDocID_;
    pClone->nLastLoc_ = nLastLoc_;
    return pClone;
}


//////////////////////////////////////////////////////////////////////////
///InMemoryPosting
int32_t InMemoryPosting::getNextChunkSize(int32_t nCurSize)
{
    int32_t newSize = max(32,(int32_t)(nCurSize + 0.5));
    return (int32_t)Utilities::LOG2_UP(newSize);
}

void InMemoryPosting::addLocation(docid_t docid, loc_t location)
{
    if (docid == nLastDocID_)
    {
        ///see it before,only position is needed
        if (!pLocList_->addPosting(location - nLastLoc_))
        {
            ///chunk is exhausted
            int32_t newSize = getNextChunkSize(pLocList_->nTotalSize_);
            pLocList_->addChunk(newChunk(newSize));
            pLocList_->addPosting(location - nLastLoc_);///d-gap encoding
        }
        nCurTermFreq_++;
        nLastLoc_ = location;
    }
    else///first see it
    {
        if (nCurTermFreq_ > 0)///write previous document's term freq
        {
            if (!pDocFreqList_->addPosting(nCurTermFreq_))
            {
                ///chunk is exhausted
                int32_t newSize = getNextChunkSize(pDocFreqList_->nTotalSize_);
                pDocFreqList_->addChunk(newChunk(newSize));
                pDocFreqList_->addPosting(nCurTermFreq_);
            }
        }
        else if (nLastDocID_ == BAD_DOCID)///first see it
        {
            nLastDocID_ = 0;
        }
        if (!pDocFreqList_->addPosting(docid - nLastDocID_))
        {
            ///chunk is exhausted
            int32_t newSize = getNextChunkSize(pDocFreqList_->nTotalSize_);
            pDocFreqList_->addChunk(newChunk(newSize));
            pDocFreqList_->addPosting(docid - nLastDocID_);
        }
        if (!pLocList_->addPosting(location))
        {
            ///chunk is exhausted
            int32_t newSize = getNextChunkSize(pLocList_->nTotalSize_);
            pLocList_->addChunk(newChunk(newSize));
            pLocList_->addPosting(location);
        }

        nCTF_ += nCurTermFreq_;
        nCurTermFreq_ = 1;

        nLastDocID_ = docid;
        nLastLoc_ = location;

        nDF_++;
    }
}

void InMemoryPosting::writeDPosting(IndexOutput* pDOutput)
{
    ///write chunk data
    PostingChunk* pChunk = pDocFreqList_->pHeadChunk_;
    while (pChunk)
    {
        pDOutput->write((const char*)pChunk->data,pChunk->size);
        pChunk = pChunk->next;
    }
}

fileoffset_t InMemoryPosting::writePPosting(IndexOutput* pPOutput)
{
    ///write position posting data
    PostingChunk* pChunk = pLocList_->pHeadChunk_;
    int size = 0;
    while (pChunk)
    {
        pPOutput->write((const char*)pChunk->data,pChunk->size);
        size+=pChunk->size;
        pChunk = pChunk->next;
    }
    fileoffset_t poffset = pPOutput->getFilePointer();
    pPOutput->writeVLong(pLocList_->getRealSize());///<ChunkLength(VInt64)>
    return poffset;
}

void InMemoryPosting::writeDescriptor(IndexOutput* pDOutput,fileoffset_t poffset)
{
    ///begin write posting descriptor
    pDOutput->writeVLong(pDocFreqList_->getRealSize());///<PostingLength(VInt64)>
    pDOutput->writeVInt(nDF_); 						///<DF(VInt32)>
    pDOutput->writeVLong(nCTF_);						///<CTF(VInt64)>
    pDOutput->writeVLong(poffset);						///<PositionPointer(VInt64)>
    ///end write posting descriptor

    pDOutput->writeVInt(1); 							///<ChunkCount(VInt32)>
    ///begin write chunk descriptor
    pDOutput->writeVLong(pDocFreqList_->getRealSize());///<ChunkLength(VInt64)>
    pDOutput->writeVInt(nLastDocID_);					///<LastDocID(VInt32)>
    ///end write posting descriptor
}

void InMemoryPosting::flushLastDoc(bool bTruncTail)
{
    if(!pMemCache_)
        return;

    if (nCurTermFreq_ > 0)
    {
        if (!pDocFreqList_->addPosting(nCurTermFreq_))
        {
            int32_t newSize = getNextChunkSize(pDocFreqList_->nTotalSize_);
            pDocFreqList_->addChunk(newChunk(newSize));
            pDocFreqList_->addPosting(nCurTermFreq_);
        }
        if (bTruncTail)
        {
            pDocFreqList_->truncTailChunk();///update real size
            pLocList_->truncTailChunk();///update real size
        }
        nCTF_ += nCurTermFreq_;
        nCurTermFreq_ = 0;
    }
    else if (bTruncTail)
    {
        pDocFreqList_->truncTailChunk();///update real size
        pLocList_->truncTailChunk();///update real size
    }
}



int32_t InMemoryPosting::decodeNext(uint32_t* pPosting,int32_t length)
{

#define ISCHUNKOVER_D()\
        if(pDChunk > pDChunkEnd)\
        {\
            pDS_->decodingDChunk = pDS_->decodingDChunk->next;\
            if(!pDS_->decodingDChunk)\
                break;\
            pDS_->decodingDChunkPos = 0;\
            pDChunk = &(pDS_->decodingDChunk->data[pDS_->decodingDChunkPos]);\
            pDChunkEnd = &(pDS_->decodingDChunk->data[pDS_->decodingDChunk->size-1]);\
        }

    ///flush last document
    flushLastDoc(false);
    if (!pDS_)
    {
        pDS_ = new InMemoryPosting::DecodeState;
        pDS_->decodingDChunk = pDocFreqList_->pHeadChunk_;
        pDS_->decodingDChunkPos = 0;
        pDS_->lastDecodedDocID = 0;
        pDS_->decodedDocCount = 0;
        pDS_->decodingPChunk = pLocList_->pHeadChunk_;
        pDS_->decodingPChunkPos = 0;
        pDS_->lastDecodedPos = 0;
        pDS_->decodedPosCount = 0;
    }

    uint32_t* pDoc = pPosting;
    uint32_t* pFreq = pPosting + (length >> 1);

    int32_t left = nDF_ - pDS_->decodedDocCount;
    if (left <= 0)
        return -1;
    if (length > left*2)
        length = left*2;
    left = (length>>1);

    uint8_t* pDChunk = &(pDS_->decodingDChunk->data[pDS_->decodingDChunkPos]);
    uint8_t* pDChunkEnd = &(pDS_->decodingDChunk->data[pDS_->decodingDChunk->size-1]);

    int32_t count = 0;
    docid_t did = pDS_->lastDecodedDocID;
    while (count < left)
    {
        ISCHUNKOVER_D();
        did += CompressedPostingList::decodePosting32(pDChunk);

        if(!pDocFilter_ || !pDocFilter_->test((size_t)did))
        {
            *pDoc++ = did;
			
            ISCHUNKOVER_D();
		
            *pFreq++ = CompressedPostingList::decodePosting32(pDChunk);					
        }
        else
        {
            ///this doc is deleted
            ISCHUNKOVER_D();
            CompressedPostingList::decodePosting32(pDChunk);
        }

        count++;
    }
    ///update state
    pDS_->decodedDocCount += count;
    pDS_->lastDecodedDocID = did;
    pDS_->decodingDChunkPos = (int32_t)(pDChunk - pDS_->decodingDChunk->data);

    return (int32_t)(pDoc - pPosting);
}

void InMemoryPosting::decodeNextPositions(uint32_t* pPosting,int32_t length)
{
    uint8_t* pPChunk = &(pDS_->decodingPChunk->data[pDS_->decodingPChunkPos]);
    uint8_t* pPChunkEnd = &(pDS_->decodingPChunk->data[pDS_->decodingPChunk->size-1]);

    uint32_t* pPos = pPosting;
    loc_t loc = pDS_->lastDecodedPos;
    int32_t  nDecoded = 0;
    while (nDecoded < length)
    {
        if (pPChunk > pPChunkEnd)
        {
            pDS_->decodingPChunk = pDS_->decodingPChunk->next;
            if (!pDS_->decodingPChunk)
                break;
            pDS_->decodingPChunkPos = 0;
            pPChunk = &(pDS_->decodingPChunk->data[pDS_->decodingPChunkPos]);
            pPChunkEnd = &(pDS_->decodingPChunk->data[pDS_->decodingPChunk->size-1]);
        }

        loc += CompressedPostingList::decodePosting32(pPChunk);
        if (pPos)
        {
            *pPos = loc;
            pPos++;
        }
        nDecoded++;
    }
    pDS_->decodedPosCount += nDecoded;
    pDS_->lastDecodedPos = loc;
}

void InMemoryPosting::decodeNextPositions(uint32_t* pPosting,uint32_t* pFreqs,int32_t nFreqs)
{
    uint8_t* pPChunk = &(pDS_->decodingPChunk->data[pDS_->decodingPChunkPos]);
    uint8_t* pPChunkEnd = &(pDS_->decodingPChunk->data[pDS_->decodingPChunk->size-1]);

    uint32_t* pPos = pPosting;
    loc_t loc = pDS_->lastDecodedPos;
    uint32_t  nTotalDecoded = 0;
    uint32_t  nCurDecoded;
    for (int32_t nF = 0; nF < nFreqs;nF++)
    {
        nCurDecoded = 0;
        while (nCurDecoded < pFreqs[nF])
        {
            if (pPChunk > pPChunkEnd)
            {
                pDS_->decodingPChunk = pDS_->decodingPChunk->next;
                if (!pDS_->decodingPChunk)
                    break;
                pDS_->decodingPChunkPos = 0;
                pPChunk = &(pDS_->decodingPChunk->data[pDS_->decodingPChunkPos]);
                pPChunkEnd = &(pDS_->decodingPChunk->data[pDS_->decodingPChunk->size-1]);
            }

            loc += CompressedPostingList::decodePosting32(pPChunk);
            if (pPos)
            {
                *pPos = loc;
                pPos++;
            }
            nCurDecoded++;
        }
        pDS_->lastDecodedPos = loc = 0;
        nTotalDecoded += nCurDecoded;
    }

    pDS_->decodedPosCount += nTotalDecoded;
    pDS_->lastDecodedPos = loc;
}

void InMemoryPosting::resetPosition()
{
    pDS_->lastDecodedPos = 0;
}
//////////////////////////////////////////////////////////////////////////
///OnDiskPosting
OnDiskPosting::OnDiskPosting(InputDescriptor* pInputDescriptor,fileoffset_t poffset)
        :pInputDescriptor_(pInputDescriptor)
        ,nBufSize_(0)
{
    postingOffset_ = poffset;
    reset(postingOffset_);
}
OnDiskPosting::OnDiskPosting()
{
    nBufSize_ = 0;
    postingOffset_ = 0;
    postingDesc_.length = 0;
    postingDesc_.df = 0;
    postingDesc_.ctf = 0;
    postingDesc_.poffset = 0;

    chunkDesc_.length = 0;
    chunkDesc_.lastdocid = 0;

    nPPostingLength_ = 0;

    ds_.decodedDocCount = 0;
    ds_.lastDecodedDocID = 0;
    ds_.decodedPosCount= 0;
    ds_.lastDecodedPos = 0;
}
OnDiskPosting::~OnDiskPosting()
{
}

Posting* OnDiskPosting::clone()
{
    return NULL;
}

void OnDiskPosting::reset(fileoffset_t newOffset)
{
    postingOffset_ = newOffset;
    IndexInput* pDPInput = pInputDescriptor_->getDPostingInput();
    //IndexInput should be reset because the internal buffer should be clear when a new posting is needed to be read
    pDPInput->reset();
    pDPInput->seekInternal(newOffset);///not seek(), because seek() may trigger a large data read event.

    ///read descriptor of posting list <PostingDescriptor>
    uint8_t buf[512];
    uint8_t* u = buf;
    pDPInput->readInternal((char*)buf,512,false);
    postingDesc_.length = CompressedPostingList::decodePosting64(u);	///<PostingLength(VInt64)>
    postingDesc_.df = CompressedPostingList::decodePosting32(u);		///<DF(VInt32)>
    postingDesc_.ctf = CompressedPostingList::decodePosting64(u);		///<CTF(VInt64)>
    postingDesc_.poffset = CompressedPostingList::decodePosting64(u);	///PositionPointer(VInt64)

    CompressedPostingList::decodePosting32(u);///<ChunkCount(VInt32)>
    ///read first chunk descriptor of posting list <ChunkDescriptor>
    chunkDesc_.length = CompressedPostingList::decodePosting64(u);	///<ChunkLength(VInt64)>
    chunkDesc_.lastdocid = CompressedPostingList::decodePosting32(u);	///<LastDocID(VInt32)>
    IndexInput* pPPInput = pInputDescriptor_->getPPostingInput();
    if (pPPInput)
    {
        pPPInput->reset();
        pPPInput->seekInternal(postingDesc_.poffset);///not seek(), because seek() may trigger a large data read event.
        pPPInput->readInternal((char*)buf,8,false);
        u = buf;
        nPPostingLength_ = CompressedPostingList::decodePosting64(u); ///<ChunkLength(VInt64)>
        pPPInput->seek(postingDesc_.poffset - nPPostingLength_);///seek to the begin of position posting data
    }
    else
    {
        nPPostingLength_ = 0;
    }

    pDPInput->seek(newOffset - postingDesc_.length);	///seek to the begin of posting data

    ds_.decodedDocCount = 0;
    ds_.lastDecodedDocID = 0;
    ds_.decodedPosCount= 0;
    ds_.lastDecodedPos = 0;
}

int32_t OnDiskPosting::decodeNext(uint32_t* pPosting,int32_t length)
{
    int32_t left = postingDesc_.df - ds_.decodedDocCount;
    if (left <= 0)
        return -1;
    uint32_t* pDoc = pPosting;
    uint32_t* pFreq = pPosting + (length>>1);

    if (length > left*2)
        length = left*2;
    left = (length>>1);

    IndexInput* pDPostingInput = getInputDescriptor()->getDPostingInput();

    int32_t count = 0;
    docid_t did = ds_.lastDecodedDocID;

    while (count < left)
    {
        did += pDPostingInput->readVInt();

        if(!pDocFilter_ || !pDocFilter_->test((size_t)did))
        {
            *pDoc++ = did;
            *pFreq++ = pDPostingInput->readVInt();
        }
        else
        {
            ///this doc is deleted
            pDPostingInput->readVInt();
        }				

        count++;
    }

    ///update state
    ds_.decodedDocCount += count;
    ds_.lastDecodedDocID = did;

    return (int32_t)(pDoc - pPosting);
}
void OnDiskPosting::decodeNextPositions(uint32_t* pPosting,int32_t length)
{
    if (length <= 0)
        return;
    IndexInput* pPPostingInput = getInputDescriptor()->getPPostingInput();

    int32_t nDecoded = 0;
    loc_t loc = ds_.lastDecodedPos;
    uint32_t* pPos = pPosting;
    while (nDecoded < length)
    {
        loc += pPPostingInput->readVInt();
        if (pPos)
        {
            *pPos = loc;
            pPos++;
        }
        nDecoded++;
    }

    ds_.decodedPosCount += nDecoded;
    ds_.lastDecodedPos = loc;
}
void OnDiskPosting::decodeNextPositions(uint32_t* pPosting,uint32_t* pFreqs,int32_t nFreqs)
{
    IndexInput*	pPPostingInput = getInputDescriptor()->getPPostingInput();

    uint32_t nTotalDecoded = 0;
    uint32_t nCurDecoded = 0;
    loc_t loc = ds_.lastDecodedPos;
    uint32_t* pPos = pPosting;
    for (int32_t nF = 0;nF < nFreqs;nF++)
    {
        nCurDecoded = 0;
        while (nCurDecoded < pFreqs[nF])
        {
            loc += pPPostingInput->readVInt();
            if (pPos)
            {
                *pPos = loc;
                pPos++;
            }
            nCurDecoded++;
        }
        nTotalDecoded += nCurDecoded;
        loc = 0;
    }

    ds_.decodedPosCount += nTotalDecoded;
    ds_.lastDecodedPos = loc;
}
void OnDiskPosting::resetPosition()
{
    ds_.lastDecodedPos = 0;
}

void OnDiskPosting::reset()
{
    reset(postingOffset_);
}

size_t OnDiskPosting::setBuffer(int32_t* buffer,size_t nBufSize)
{
    size_t nBufUsed = nBufSize*sizeof(int32_t);
    if (nBufUsed <= 2*INDEXINPUT_BUFFSIZE)
    {
        nBufSize_ = 0;
        return 0;
    }

    IndexInput* pDInput = getInputDescriptor()->getDPostingInput();
    IndexInput* pPInput = getInputDescriptor()->getPPostingInput();
    if ((int64_t)nBufUsed > (postingDesc_.length + nPPostingLength_))
    {
        nBufUsed = (size_t)(postingDesc_.length + nPPostingLength_);
        pDInput->setBuffer((char*)buffer,(size_t)postingDesc_.length);
        if (pPInput)
            pPInput->setBuffer((char*)buffer + postingDesc_.length,(size_t)nPPostingLength_);
    }
    else
    {
        size_t nDSize = nBufUsed/2;
        if ((int64_t)nDSize > postingDesc_.length)
            nDSize = (size_t)postingDesc_.length;
        pDInput->setBuffer((char*)buffer,nDSize);
        if (pPInput)
            pPInput->setBuffer((char*)buffer + nDSize,nBufUsed - nDSize);
    }
    nBufSize_ = nBufUsed;
    return (nBufUsed + sizeof(int32_t) - 1)/sizeof(int32_t);
}

