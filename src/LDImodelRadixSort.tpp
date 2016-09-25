#include "../include/LDImodel.hpp"
#include "../include/Utils.hpp"
#include <ctime>

template<typename T1, typename T2, typename T3>
void LDImodel<T1, T2, T3>::transpose(GLuint ssbo_in, GLuint ssbo_out, unsigned int size_x, unsigned int size_y)
{
    unsigned int block_dim = 16;
    m_transposeShader.use();
    bindSSBO(ssbo_in, 15);
    initSSBO<GLuint>(&ssbo_out, 16*m_nbBlocks, 16);
    glUniform1ui(glGetUniformLocation(m_transposeShader.program, "MAT_HEIGHT"), size_y);
    glUniform1ui(glGetUniformLocation(m_transposeShader.program, "MAT_WIDTH"), size_x);
    glDispatchCompute(std::ceil(float(size_x)/block_dim), std::ceil(float(size_y)/block_dim), 1);
    glFinish();
}

//the two buffers have to be initialized
template<typename T1, typename T2, typename T3>
void LDImodel<T1, T2, T3>::radixSort(GLuint ssbo_input, GLuint ssbo_output, unsigned int nbElements,
                                     unsigned int significant_bits)
{
    GLuint ssbo_oData, ssbo_radixCounters, ssbo_radixBlockOffsets, ssbo_radixScannedCounters;
    unsigned int nbBlocks = std::ceil((float)nbElements/1024.0f);
    initSSBO<GLuint>(&ssbo_oData, nbBlocks*1024, 19);
    initSSBO<GLuint>(&ssbo_radixCounters, 16*nbBlocks, 20);
    initSSBO<GLuint>(&ssbo_radixBlockOffsets, 16*nbBlocks, 21);
    initSSBO<GLuint>(&ssbo_radixScannedCounters, 16*nbBlocks, 1);
    unsigned int nb_bit_blocks = std::ceil(float(significant_bits)/4.0f);
    for(unsigned int i=0; i<nb_bit_blocks; i++)
    {
        unsigned int start_bit = i<<2;
        unsigned int end_bit = (i+1)<<2;
        if(start_bit == 0)
            cudppRadixSort(ssbo_input, ssbo_oData, nbElements, start_bit, end_bit);
        else
            cudppRadixSort(ssbo_output, ssbo_oData, nbElements, start_bit, end_bit);
        cudppFindRadixOffsets(ssbo_oData, ssbo_radixCounters, ssbo_radixBlockOffsets, start_bit, end_bit,
                              nbBlocks);
        samScan(ssbo_radixCounters, ssbo_radixScannedCounters, nbElements);
        cudppReorderData(ssbo_oData, ssbo_output, ssbo_radixScannedCounters, ssbo_radixBlockOffsets,
                         nbElements, start_bit, end_bit, nbBlocks);
    }

    glDeleteBuffers(1, &ssbo_radixCounters);
    glDeleteBuffers(1, &ssbo_radixScannedCounters);
    glDeleteBuffers(1, &ssbo_oData);
    glDeleteBuffers(1, &ssbo_radixBlockOffsets);
}

template<typename T1, typename T2, typename T3>
void LDImodel<T1, T2, T3>::radixSort(std::vector<GLuint> & vInput, unsigned int significant_bits)
{
    GLuint ssbo_input, ssbo_output;
    //vector -> DRAM
    glGenBuffers(1, &ssbo_input);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_input);
    glBufferData(GL_SHADER_STORAGE_BUFFER, vInput.size()*sizeof(GLuint), vInput.data(), GL_STREAM_READ);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 18, ssbo_input);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glFinish();
    //preparing the output
    initSSBO<GLuint>(&ssbo_output, vInput.size(), 3);
    //actual sort
    radixSort(ssbo_input, ssbo_output, vInput.size(), significant_bits);
    //fetch the result
    std::vector<GLuint> tmpSorted;
    fetchSSBO<GLuint>(ssbo_output, vInput.size(), tmpSorted);
    std::copy(tmpSorted.begin(), tmpSorted.end(), vInput.begin());
    //free the memory
    glDeleteBuffers(1, &ssbo_input);
    glDeleteBuffers(1, &ssbo_output);
}

template<typename T1, typename T2, typename T3>
void LDImodel<T1, T2, T3>::testRadixSort()
{
    //data preparation
    int nbElements = 30332666;
    std::cout<<"sorting "<<nbElements<<" elements..."<<std::endl;
    std::vector<GLuint> iData(nbElements);
    for(int i=0; i<iData.size(); i++)
    {
        iData[i] = rand()%100;
    }
    radixSort(iData, 8);

    bool check = true;
    for(int i=1; i<iData.size(); i++)
    {
        if(i > 0)
        {
            if(i%1024 != 0)
            {
                bool tmpCheck = (iData[i]&0xff) >= (iData[i-1]&0xff);
                check &= tmpCheck;
            }
        }
        //std::cout<<iData[i]<<std::endl;
    }
    std::cerr<<"RADIX_SORT_TEST: "<<(check ? "PASSED" : "FAILED")<<std::endl;
}

template<typename T1, typename T2, typename T3>
void LDImodel<T1, T2, T3>::cudppRadixSort(GLuint ssbo_iData, GLuint ssbo_oData, unsigned int nbElements,
                              unsigned int start_bit, unsigned int end_bit)
{
    m_cudppRadixSortShader.use();
    int block_size = 256;
    int nbBlocks = std::ceil(float(nbElements)/float((4*block_size)));
    bindSSBO(ssbo_iData, 18);
    bindSSBO(ssbo_oData, 19);
    glUniform1ui(glGetUniformLocation(m_cudppRadixSortShader.program, "START_BIT"), start_bit);
    glUniform1ui(glGetUniformLocation(m_cudppRadixSortShader.program, "END_BIT"), end_bit);
    glUniform1ui(glGetUniformLocation(m_cudppRadixSortShader.program, "NB_ELEMENTS"), nbElements);

    glDispatchCompute(nbBlocks, 1, 1);
    glFinish();
}

template<typename T1, typename T2, typename T3>
void LDImodel<T1, T2, T3>::cudppFindRadixOffsets(GLuint ssbo_oData, GLuint ssbo_radixCounters, GLuint ssbo_radixBlockOffsets,
                                     unsigned int start_bit, unsigned int end_bit, unsigned int nbBlocks)
{
    m_cudppFindRadixOffsetsShader.use();
    bindSSBO(ssbo_oData, 19);
    bindSSBO(ssbo_radixCounters, 20);
    bindSSBO(ssbo_radixBlockOffsets, 21);
    glUniform1ui(glGetUniformLocation(m_cudppFindRadixOffsetsShader.program, "TOTAL_BLOCKS"), nbBlocks);
    glUniform1ui(glGetUniformLocation(m_cudppFindRadixOffsetsShader.program, "START_BIT"), start_bit);
    glUniform1ui(glGetUniformLocation(m_cudppFindRadixOffsetsShader.program, "CTA_SIZE"), 256);

    glDispatchCompute(nbBlocks, 1, 1);
    glFinish();
}

template<typename T1, typename T2, typename T3>
void LDImodel<T1, T2, T3>::samScan(GLuint ssbo_input, GLuint ssbo_output, int nb_elements)
{
    int nb_blocks = std::max(std::ceil(float(nb_elements)/1024.0f), 1.0f);

    m_samScanShader.use();
    bindSSBO(ssbo_input, 0);
    bindSSBO(ssbo_output, 1);
    GLuint ssbo_gcarry, ssbo_gwait;
    initSSBO<GLuint>(&ssbo_gcarry, nb_blocks, 2);
    initSSBO<GLuint>(&ssbo_gwait, nb_blocks, 3);
    bindSSBO(ssbo_gcarry, 2);
    bindSSBO(ssbo_gwait, 3);
    glUniform1ui(glGetUniformLocation(m_samScanShader.program, "NB_ELEMENTS"), nb_elements);

    glDispatchCompute(nb_blocks, 1, 1);
    glFinish();
    glDeleteBuffers(1, &ssbo_gcarry);
    glDeleteBuffers(1, &ssbo_gwait);
}

template<typename T1, typename T2, typename T3>
void LDImodel<T1, T2, T3>::cudppReorderData(GLuint ssbo_keys, GLuint ssbo_outKeys, GLuint ssbo_offsets,
                                GLuint ssbo_blockOffsets, unsigned int nb_elements, unsigned int start_bit,
                                unsigned int end_bit, unsigned int nbBlocks)
{
    m_cudppReorderDataShader.use();
    bindSSBO(ssbo_keys, 0);
    bindSSBO(ssbo_offsets, 1);
    bindSSBO(ssbo_blockOffsets, 2);
    bindSSBO(ssbo_outKeys, 3);
    glUniform1ui(glGetUniformLocation(m_cudppReorderDataShader.program, "TOTAL_BLOCKS"), nbBlocks);
    glUniform1ui(glGetUniformLocation(m_cudppReorderDataShader.program, "NB_ELEMENTS"), nb_elements);
    glUniform1ui(glGetUniformLocation(m_cudppReorderDataShader.program, "START_BIT"), start_bit);
    glUniform1ui(glGetUniformLocation(m_cudppReorderDataShader.program, "CTA_SIZE"), 256);

    glDispatchCompute(nbBlocks, 1, 1);
    glFinish();
}
