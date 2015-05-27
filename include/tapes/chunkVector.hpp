/**
 * CoDiPack, a Code Differentiation Package
 *
 * Copyright (C) 2015 Chair for Scientific Computing, TU Kaiserslautern
 *
 * This file is part of CoDiPack.
 *
 * CoDiPack is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 2 of the
 * License, or (at your option) any later version.
 *
 * CoDiPack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU
 * General Public License along with CoDiPack.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: TODO
 */
#pragma once

#include <tuple>
#include <vector>

#include "chunk.hpp"

namespace codi {

  /**
   * @brief Implementation for a terminal sequence chunk vector
   *
   * This interface provides the basic implementation for a terminal point
   * in a chain of chunk vectors.
   */
  struct EmptyChunkVector {
    /**
     * @brief Position without any data.
     */
    struct Position {};

    /**
     * @brief Empty position.
     * @return Empty position
     */
    inline Position getPosition() {
      return Position();
    }

    /**
     * @brief Will do nothing.
     */
    inline void reset(const Position& CODI_UNUSED(pos)) {}
  };

  /**
   * @brief A vector which manages chunks of data for the taping process.
   *
   * The vector stores an array of data chunks which have all the same size.
   * The data in the chunk can be accessed in a stack like fashion. The user
   * has to check first if enough data is available. The chunk vector will
   * make shure that the current loaded chunk has enough data. The user can then
   * push as many data items as he has reserved on the chunk vector.
   *
   * The read acces to the data is provided by the function forEach, which will
   * call the provided function handle on every data item. A second option is to
   * get direct pointers to the data with the getDataAtPostion function.
   *
   * As some tapes need multiple chunk vectors, the design of the chunk vector reflects
   * this need. The user never knows when a chunk vector pushes a new chunk on the stack
   * but the users needs this information in order to know which range he can evaluate.
   * Therefor the chunk vector needs to store the position of the nested chunk vector
   * every time it pushes a new chunk. The second template argument is used for this
   * kind of usage. It gives the chunk vector the means to access the information it
   * needs.
   *
   * @tparam    ChunkData   The data the chunk vector will store.
   * @tparam NestedVector   A nested chunk vector used for position information
   *                          every time a chunk is pushed.
   */
  template<typename ChunkData, typename NestedVector = EmptyChunkVector>
  class ChunkVector {
  public:

    /**
     * @brief Position of the nested vector
     */
    typedef typename NestedVector::Position NestedPosition;

    /**
     * @brief Position of this chunk vector.
     *
     * The position also includes the position of the nested vector,
     * such that the full position of all the chunk vectors
     * is available to the user.
     */
    struct Position {
      /**
       * @brief chunk
       */
      size_t chunk; /**< Index of the chunk */
      size_t data;  /**< Data position in the chunk */

      NestedPosition inner; /**< Position of the nested chunk vector */

      /**
       * @brief Default constructor is needed if this position is used as an inner position.
       */
      Position() :
        chunk(0),
        data(0),
        inner() {}

      /**
       * @brief Create the full position for all the nested vectors.
       * @param chunk   Index of the current chunk.
       * @param  data   Index of the data in the current chunk.
       * @param inner   Position of the nexted vector.
       */
      Position(const size_t& chunk, const size_t& data, const NestedPosition& inner) :
        chunk(chunk),
        data(data),
        inner(inner) {}
    };

  private:
    std::vector<ChunkData* > chunks; /**< Array of the chunks */

    /**
     * @brief Array of nested positions for the chunks.
     *
     * This vector contains the position of the nested chunk vector. The
     * position is updated if the corresponding chunk in the chunks array
     * is loaded.
     */
    std::vector<NestedPosition> positions;

    ChunkData* curChunk; /**< The current loaded chunk. This will never be NULL */
    size_t curChunkIndex; /**< Index of the chunk which loaded. */

    size_t chunkSize; /**< Global size of the chunks. If this size is set all the chunks are resized. */

    NestedVector& nested; /**< Reference to the nested vector. */

  public:

    //TODO: maybe create a constructor without the need to supply a nested vector
    /**
     * @brief Creates one chunk and loads it.
     * @param chunkSize   The size for the chunks.
     * @param    nested   The nested chunk vector.
     */
    ChunkVector(const size_t& chunkSize, NestedVector& nested) :
      chunks(),
      positions(),
      curChunk(NULL),
      curChunkIndex(0),
      chunkSize(chunkSize),
      nested(nested)
    {
      curChunk = new ChunkData(chunkSize);
      chunks.push_back(curChunk);
      positions.push_back(nested.getPosition());
    }

    /**
     * @brief Deletes all chunks.
     */
    ~ChunkVector() {
      for(size_t i = 0; i < chunks.size(); ++i) {
        delete chunks[i];
      }
    }

    /**
     * @brief Sets the global chunk size and resizes all chunks.
     * @param chunkSize   The new chunk size.
     */
    void setChunkSize(const size_t& chunkSize) {
      this->chunkSize = chunkSize;

      for(size_t i = 0; i < chunks.size(); ++i) {
        chunks[i]->resize(this->chunkSize);
      }
    }

    /**
     * @brief Ensures that enough chunks are allocated so that totalSize data items can be stored.
     * @param totalSize   The number of data items which should be available.
     */
    void resize(const size_t& totalSize) {
      size_t noOfChunks = totalSize / chunkSize;
      if(0 != totalSize % chunkSize) {
        noOfChunks += 1;
      }

      for(size_t i = chunks.size(); i < noOfChunks; ++i) {
        chunks.push_back(new ChunkData(chunkSize));
        positions.push_back(nested.getPosition());
      }
    }

    /**
     * @brief Loads the next chunk.
     *
     * If the current chunk was the last chunk in the array a new chunk is created.
     * Otherwise the old chunk is reset and loaded as current chunk.
     *
     * Always the position of the nested chunk vector is stored.
     */
    inline void nextChunk() {
      curChunk->store();

      curChunkIndex += 1;
      if(chunks.size() == curChunkIndex) {
        curChunk = new ChunkData(chunkSize);
        chunks.push_back(curChunk);
        positions.push_back(nested.getPosition());
      } else {
        curChunk = chunks[curChunkIndex];
        curChunk->reset();
        positions[curChunkIndex] = nested.getPosition();
      }
    }

    /**
     * @brief Resets the chunk vector to the given position.
     *
     * This method will call reset on all chunks which are behind the
     * given position.
     *
     * It calls also reset on the nested chunk vector.
     *
     * @param pos   The position to reset to.
     */
    void reset(const Position& pos) {
      assert(pos.chunk < chunks.size());
      assert(pos.data < chunkSize);

      for(size_t i = curChunkIndex; i > pos.chunk; --i) {
        chunks[i]->reset();
      }

      curChunk = chunks[pos.chunk];
      curChunk->load();
      curChunk->setUsedSize(pos.data);
      curChunkIndex = pos.chunk;

      nested.reset(pos.inner);
    }

    /**
     * @brief Resets the complete chunk vector.
     */
    void reset() {
      reset(Position());
    }

    /**
     * @brief Checks if the current chunk has enough items left.
     *
     * If the chunk has not enough ites left, the next chunk is
     * loaded.
     *
     * @param items   The maximum number of items to store.
     */
    inline void reserveItems(const size_t items) {
      assert(items <= chunkSize);

      if(chunkSize < curChunk->getUsedSize() + items) {
        nextChunk();
      }
    }

    /**
     * @brief Sets the data and increases the used chunk data by one.
     *
     * This method should only be called if 'reserveItems' was called
     * beforehand with enough items to accommodate to all calls to this
     * method.
     *
     * @param data  The data set to the current position in the chunk.
     */
    inline void setDataAndMove(const typename ChunkData::DataValues& data) {
      // this method should only be called if reserveItems has been called
      curChunk->setDataAndMove(data);
    }

    /**
     * @brief The position inside the data of the current chunk.
     * @return The current position in the current chunk.
     */
    inline size_t getChunkPosition() {
      return curChunk->getUsedSize();
    }

    /**
     * @brief Get the position of the chunk vector and the nested vectors.
     * @return The position of the chunk vector.
     */
    inline Position getPosition() {
      return Position(curChunkIndex, curChunk->getUsedSize(), nested.getPosition());
    }

    /**
     * @brief Get the position of the nested chunk vector when the chunk was loaded.
     *
     * @param chunkIndex  The index of the chunk for which the position is required.
     * @return The position of the nested chunk vector when the chunk was loaded.
     */
    inline NestedPosition getInnerPosition(const size_t& chunkIndex) {
      assert(chunkIndex < positions.size());
      return positions[chunkIndex];
    }

    /**
     * @brief Get a pointer to the data at the given position.
     * @param chunkIndex  The index of the chunk.
     * @param    dataPos  The index for the data in the chunk.
     * @return A pointer to the data of the chunk at the given position.
     */
    inline typename ChunkData::DataPointer getDataAtPosition(const size_t& chunkIndex, const size_t& dataPos) {
      assert(chunkIndex < chunks.size());

      return chunks[chunkIndex]->dataPointer(dataPos);
    }

    /**
     * @brief Get the number of data items used in the chunk.
     * @param chunkIndex  The chunk from which the information is extracted.
     * @return The number of data items used in the chunk.
     */
    inline size_t getChunkUsedData(const size_t& chunkIndex) {
      assert(chunkIndex < chunks.size());

      return chunks[chunkIndex]->getUsedSize();
    }

  private:
    /**
     * @brief Iterates of the data entries in the chunk.
     *
     * Iterates of the data entries and calls the function object with each data item.
     *
     * It has to hold start >= end.
     *
     * @param chunkPos  The position of the chunk.
     * @param    start  The starting point inside the data of the chunk.
     * @param      end  The end point inside the data of the chunk.
     * @param function  The function called for each data entry.
     */
    template<typename FunctionObject>
    inline void forEachData(const size_t& chunkPos, const size_t& start, const size_t& end, FunctionObject& function) {
      assert(start >= end);
      assert(chunkPos < chunks.size());

      typename ChunkData::DataPointer data;
      // we do not initialize dataPos with start - 1 because the type can be unsigned
      for(size_t dataPos = start; dataPos > end; /* decrement is done inside the loop */) {
        --dataPos; // decrement of loop variable

        data = getDataAtPosition(chunkPos, dataPos);
        function(data);
      }
    }

  public:

    /**
     * @brief Iterates of all data entries in the given range
     *
     * Iterates of the data entries and calls the function object with each data item.
     *
     * It has to hold start >= end.
     *
     * @param    start  The starting point of the range.
     * @param      end  The end point of the range.
     * @param function  The function called for each data entry.
     */
    template<typename FunctionObject>
    inline void forEach(const Position& start, const Position& end, FunctionObject& function) {
      assert(start.chunk > end.chunk || (start.chunk == end.chunk && start.data >= end.data));
      assert(start.chunk < chunks.size());

      size_t dataStart = start.data;
      for(size_t chunkPos = start.chunk; chunkPos > end.chunk; /* decrement is done inside the loop */) {

        forEachData(chunkPos, dataStart, 0, function);

        dataStart = chunks[--chunkPos]->getUsedSize(); // decrement of loop variable

      }

      forEachData(end.chunk, dataStart, end.data, function);
    }
  };
}
