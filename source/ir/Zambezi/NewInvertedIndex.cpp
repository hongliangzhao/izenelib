#include <ir/Zambezi/NewInvertedIndex.hpp>
#include <ir/Zambezi/Utils.hpp>


NS_IZENELIB_IR_BEGIN

namespace Zambezi
{

NewInvertedIndex::NewInvertedIndex(bool reverse)
    : buffer_(DEFAULT_VOCAB_SIZE)
    , pool_(NUMBER_OF_POOLS, reverse)
    , pointers_(DEFAULT_VOCAB_SIZE)
{
}

NewInvertedIndex::~NewInvertedIndex()
{
}

void NewInvertedIndex::save(std::ostream& ostr) const
{
    buffer_.save(ostr);
    pool_.save(ostr);
    dictionary_.save(ostr);
    pointers_.save(ostr);
}

void NewInvertedIndex::load(std::istream& istr)
{
    buffer_.load(istr);
    pool_.load(istr);
    dictionary_.load(istr);
    pointers_.load(istr);
}

bool NewInvertedIndex::hasValidPostingsList(uint32_t termid) const
{
    return pointers_.getHeadPointer(termid) != UNDEFINED_POINTER;
}

void NewInvertedIndex::insertDoc(
        uint32_t docid,
        const std::vector<std::string>& term_list,
        const std::vector<uint32_t>& score_list)
{
    pointers_.setDocLen(docid, term_list.size());

    for (uint32_t i = 0; i < term_list.size(); ++i)
    {
        uint32_t id = dictionary_.insertTerm(term_list[i]);
        pointers_.cf_.increment(id);
        std::vector<uint32_t>& docBuffer = buffer_.getDocidList(id);
        std::vector<uint32_t>& scoreBuffer = buffer_.getScoreList(id);

        uint32_t df = pointers_.getDf(id);
        if (df < DF_CUTOFF)
        {
            if (docBuffer.capacity() == 0)
            {
                docBuffer.reserve(DF_CUTOFF);
                scoreBuffer.reserve(DF_CUTOFF);
            }
            docBuffer.push_back(docid);
            scoreBuffer.push_back(score_list[i]);
            pointers_.df_.increment(id);
            continue;
        }

        if (docBuffer.capacity() < BLOCK_SIZE)
        {
            docBuffer.reserve(BLOCK_SIZE);
            scoreBuffer.reserve(BLOCK_SIZE);
        }

        docBuffer.push_back(docid);
        scoreBuffer.push_back(score_list[i]);
        pointers_.df_.increment(id);

        if (docBuffer.size() == docBuffer.capacity())
        {
            uint32_t nb = docBuffer.size() / BLOCK_SIZE;
            size_t pointer = buffer_.tailPointer_[id];

            for (uint32_t j = 0; j < nb; ++j)
            {
                pointer = pool_.compressAndAppend(
                        codec_,
                        &docBuffer[j * BLOCK_SIZE],
                        &scoreBuffer[j * BLOCK_SIZE],
                        BLOCK_SIZE,
                        pointer);

                if (pool_.isReverse() || pointers_.getHeadPointer(id) == UNDEFINED_POINTER)
                {
                    pointers_.setHeadPointer(id, pointer);
                }
            }

            buffer_.tailPointer_[id] = pointer;

            docBuffer.clear();
            scoreBuffer.clear();

            if (scoreBuffer.capacity() < MAX_BLOCK_SIZE)
            {
                uint32_t newLen = docBuffer.capacity() * EXPANSION_RATE;
                docBuffer.reserve(newLen);
            }
        }
    }
}

void NewInvertedIndex::flush()
{
    uint32_t term = UNDEFINED_OFFSET;
    while ((term = buffer_.nextIndex(term, DF_CUTOFF)) != UNDEFINED_OFFSET)
    {
        std::vector<uint32_t>& docBuffer = buffer_.docid_[term];
        std::vector<uint32_t>& scoreBuffer = buffer_.score_[term];

        uint32_t pos = docBuffer.size();
        size_t pointer = buffer_.tailPointer_[term];

        uint32_t nb = pos / BLOCK_SIZE;
        uint32_t res = pos % BLOCK_SIZE;

        for (uint32_t i = 0; i < nb; ++i)
        {
            pointer = pool_.compressAndAppend(
                    codec_,
                    &docBuffer[i * BLOCK_SIZE],
                    &scoreBuffer[i * BLOCK_SIZE],
                    BLOCK_SIZE,
                    pointer);

            if (pool_.isReverse() || pointers_.getHeadPointer(term) == UNDEFINED_POINTER)
            {
                pointers_.setHeadPointer(term, pointer);
            }
        }

        if (res > 0)
        {
            pointer = pool_.compressAndAppend(
                    codec_,
                    &docBuffer[nb * BLOCK_SIZE],
                    &scoreBuffer[nb * BLOCK_SIZE],
                    res,
                    pointer);

            if (pool_.isReverse() || pointers_.getHeadPointer(term) == UNDEFINED_POINTER)
            {
                pointers_.setHeadPointer(term, pointer);
            }
        }

        buffer_.tailPointer_[term] = pointer;

        docBuffer.clear();
        scoreBuffer.clear();
    }
}

uint32_t NewInvertedIndex::totalDocNum() const
{
    return pointers_.getTotalDocs();
}

void NewInvertedIndex::retrieval(
        Algorithm algorithm,
        const std::vector<std::string>& term_list,
        uint32_t hits,
        std::vector<uint32_t>& docid_list,
        std::vector<uint32_t>& score_list) const
{
    std::vector<std::pair<uint32_t, size_t> > queries;
    uint32_t minimumDf = 0xFFFFFFFF;
    for (uint32_t i = 0; i < term_list.size(); ++i)
    {
        uint32_t termid = dictionary_.getTermId(term_list[i]);
        if (termid != INVALID_ID)
        {
            size_t pointer = pointers_.getHeadPointer(termid);
            if (pointer != UNDEFINED_POINTER)
            {
                queries.push_back(std::make_pair(pointers_.getDf(termid), pointer));
                minimumDf = std::min(queries.back().first, minimumDf);
            }
        }
    }

    if (queries.empty()) return;

    if (algorithm == SVS)
    {
        std::sort(queries.begin(), queries.end());
    }

    std::vector<size_t> qHeadPointers(queries.size());
    for (uint32_t i = 0; i < queries.size(); ++i)
    {
        qHeadPointers[i] = queries[i].second;
    }

    if (algorithm == SVS)
    {
        pool_.intersectSvS(qHeadPointers, minimumDf, hits, docid_list, score_list);
    }
    else if (algorithm == WAND)
    {
        pool_.wand(qHeadPointers, 0, hits, docid_list, score_list);
    }
}

}

NS_IZENELIB_IR_END
