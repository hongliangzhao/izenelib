#ifndef _MT_TRIE_H_
#define _MT_TRIE_H_

#include <iostream>

#include "hdb_trie.hpp"

NS_IZENELIB_AM_BEGIN

template <typename StringType>
class MtTrie
{

public:

    MtTrie(const std::string& name)
    :   name_(name)
    {
    }

    /**
     * @brief Main control flow, invoking multiple threads processing string collection.
     * @param   filename - input file that contains all terms.
     *          partitionNum - how many partitions do we divide input terms into.
     *          threadNum - how many threads do we start, each thread could process
     *                      one or more partitions.
     */
    void executeTask(const std::string& inputName, int partitionNum, int threadNum) {
        inputName_ = inputName;
        partitionNum_ = partitionNum;
        threadNum_ = threadNum;
        if( partitionNum_ <=0 || threadNum_<= 0)
            throw std::runtime_error("wrong partitionNum or threadNum");
        if( partitionNum % threadNum )
            throw std::runtime_error("partitionNum should be multiple of threadNum");

        // Sample 1000 records to estimate segments.
        std::cout << "MtTrie["<< name_ << "] Start preprocess" << std::endl;
        preprocess();

        std::cout << "MtTrie[" << name_ << "] Start building tries with " << threadNum_
            << " threads on " << partitionNum_ << " partitions" << std::endl;
        // multi-threaded building tries on partitions
        for( int i=0; i < threadNum_; i++ )
          workerThreads_.create_thread(boost::bind(&MtTrie::taskBody, this, i) );
        workerThreads_.join_all();

        // merging tries on partitions into one
        std::cout << "MtTrie[" << name_ << "] Start merge tries " << std::endl;
        mergeTries();
    }

    /**
     * @brief Get a list of values whose keys match the wildcard query. Only "*" and
     * "?" are supported currently, legal input looks like "ea?th", "her*", or "*ear?h".
     * @return true at leaset one result found.
     *         false nothing found.
     */
    bool findRegExp(const StringType& regexp,
        std::vector<StringType>& valueList)
    {
        return false;
    }

protected:

    /**
     * @brief Get prefix of files associated with a specific partition.
     */
    std::string getPartitionName(int partitionId) {
        return name_ + ".partition" + boost::lexical_cast<std::string>(partitionId);
    }

    /**
     * @brief Skip over all input terms and select some samples to get
     *        statistical information such as file's length,
     *        how many terms it contains, how to split this file
     *        to allow multiple threads rescan it at the same time.
     *        how to divide distinct terms into partitions, and so on.
     */
    void computeBoundaries(float sampleRate) {

        std::ifstream input;
        input.open(inputName_.c_str(), std::ofstream::in|std::ofstream::binary );
        if(input.fail())
            throw std::runtime_error("failed open file " + inputName_);

        // normalize
        if(sampleRate < 1.0e-10) {
             sampleRate = 1.0e-10;
        } else if(sampleRate > 1.0) {
            sampleRate = 1.0;
        }
        int sampleInterval = (int)(1/sampleRate);

        long line = 0;
        int buffersize = 0;
        char charBuffer[256];
        StringType term;

        std::set<StringType> samples;

        while(!input.eof())
        {
            input.read((char*)&buffersize, sizeof(int));
            if( line%sampleInterval || buffersize > 256 ) {
                input.seekg(buffersize, ifstream::cur);
            } else {
                // selected as a sample
                input.read( charBuffer, buffersize );
                term.assign( std::string(charBuffer,buffersize) );
                if(term.size() > 0) {
                    ///TODO
                    samples.insert(term);
                }
            }
            line ++;
        }

        fileLength_ = input.tellg();
        fileLines_ = line;
        input.close();

        // caculate boundaries between partitions
        size_t interval = samples.size()/partitionNum_;
        int count = 0;
        typename std::set<StringType>::iterator it = samples.begin();

        while(count < partitionNum_-1) {
            for(size_t i=0; i<interval; i++)
                it++;
            boundaries_.push_back(*it);
            count ++;
        }
    }

    /**
     * @brief Split orignal input files into multiple partitions.
     */
    void splitInput() {

        std::ifstream input;
        input.open(inputName_.c_str(), std::ifstream::in|std::ifstream::binary );
        if(input.fail())
            throw std::runtime_error("failed open file " + inputName_);

        std::ofstream* output = new std::ofstream[partitionNum_];
        for(int i = 0; i<partitionNum_; i++ ) {
            std::string outputName = getPartitionName(i) + ".input";
            output[i].open(outputName.c_str(), std::ofstream::out|std::ofstream::binary);
            if(output[i].fail())
                throw std::runtime_error("failed open file " + outputName);
        }

        StringType term;
        int buffersize = 0;
        char charBuffer[256];

        while(!input.eof())
        {
            input.read((char*)&buffersize, sizeof(int));
            // skip too long term
            if(buffersize > 256) {
                input.seekg(buffersize, std::ifstream::cur);
                continue;
            }
            input.read( charBuffer, buffersize );
            term.assign( std::string(charBuffer,buffersize) );

            if(term.size() > 0) {
                if(term.compare(boundaries_[0]) < 0) {
                    output[0].write((char*)&buffersize, sizeof(int));
                    output[0].write(charBuffer, buffersize);
                } else if(term.compare(boundaries_[boundaries_.size()-1]) >= 0) {
                    output[boundaries_.size()].write((char*)&buffersize, sizeof(int));
                    output[boundaries_.size()].write(charBuffer, buffersize);
                } else {
                    for(size_t i=1; i<boundaries_.size(); i++) {
                        int c1 = term.compare(boundaries_[i-1]);
                        int c2 = term.compare(boundaries_[i]);
                        if(c1 >=0 && c2<0 ) {
                            output[i].write((char*)&buffersize, sizeof(int));
                            output[i].write(charBuffer, buffersize);
                            break;
                        }
                    }
                }
            }
        }
        input.close();
        for(int i = 0; i<partitionNum_; i++ )
            output[i].close();
        delete[] output;
    }

    void preprocess() {
        computeBoundaries(0.001);
        splitInput();
    }

    void taskBody(int partitionId) {
        std::string inputName = getPartitionName(partitionId) + ".input";
        std::string trieName = getPartitionName(partitionId) + ".trie";

        std::ifstream input;
        input.open(inputName.c_str(), std::ifstream::in|std::ifstream::binary );
        if(input.fail())
            throw std::runtime_error("failed open file " + inputName);

        PartitionTrie2<StringType, int> trie(trieName);
        trie.open();

        // Insert all boundaries into trie, to ensure NodeIDs are consistent
        // in all tries, the details of reasoning and proof see MtTrie TR.
        for(size_t i =0; i<boundaries_.size(); i++) {
            trie.insert(boundaries_[i], 0);
        }

        // Insert all terms in this partition
        StringType term;
        int buffersize = 0;
        char charBuffer[256];
        while(!input.eof())
        {
            input.read((char*)&buffersize, sizeof(int));
            input.read( charBuffer, buffersize );
            term.assign( std::string(charBuffer,buffersize) );
            trie.insert(term, 0);
        }
        trie.optimize();

        trie.close();
        input.close();
    }

    void mergeTries() {
        std::string trieName = name_ + ".trie";

        PartitionTrie2<StringType, int> trie(trieName);
        trie.open();
        for(size_t i =0; i<boundaries_.size(); i++) {
            trie.insert(boundaries_[i], 0);
        }
        int64_t nextNID = trie.nextNodeID();

        for(int i=0; i<partitionNum_; i++) {
            std::string p = getPartitionName(i) + ".trie";
            PartitionTrie2<StringType, int> pt(p);
            pt.open();
            trie.concatenate(pt,nextNID);
            pt.close();
        }

        trie.optimize();
        trie.close();
    }

private:

    std::string name_;

    std::string inputName_;

    int partitionNum_;

    int threadNum_;

    /// @brief boundaries between partitions,
    /// there are partitionNum_-1 elements.
    std::vector<StringType> boundaries_;

    /// @brief file length in bytes.
    size_t fileLength_;

    /// @brief how many term occurences.
    size_t fileLines_;

    boost::thread_group workerThreads_;

};

NS_IZENELIB_AM_END

#endif
