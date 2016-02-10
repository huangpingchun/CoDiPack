/*
 * CoDiPack, a Code Differentiation Package
 *
 * Copyright (C) 2015 Chair for Scientific Computing (SciComp), TU Kaiserslautern
 * Homepage: http://www.scicomp.uni-kl.de
 * Contact:  Prof. Nicolas R. Gauger (codi@scicomp.uni-kl.de)
 *
 * Lead developers: Max Sagebaum, Tim Albring (SciComp, TU Kaiserslautern)
 *
 * This file is part of CoDiPack (http://www.scicomp.uni-kl.de/software/codi).
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
 * Authors: Max Sagebaum, Tim Albring, (SciComp, TU Kaiserslautern)
 */

#pragma once

#include <iostream>
#include <cstddef>
#include <tuple>

#include "../activeReal.hpp"
#include "chunk.hpp"
#include "chunkVector.hpp"
#include "externalFunctions.hpp"
#include "reverseTapeInterface.hpp"

/**
 * @brief Global namespace for CoDiPack - Code Differentiation Package
 */
namespace codi {

  /**
   * @brief Helper struct to define the nested chunk vectors for the JacobiIndexTape.
   *
   * See JacobiIndexTape for details.
   */
  template <typename Real, typename IndexHandler>
  struct ChunkIndexTapeTypes {
    typedef Real RealType;
    typedef IndexHandler IndexHandlerType;

    /** @brief The data for each statement. */
    typedef Chunk2<StatementInt, typename IndexHandler::IndexType> StatementChunk;
    /** @brief The chunk vector for the statement data. */
    typedef ChunkVector<StatementChunk, EmptyChunkVector> StatementVector;

    /** @brief The data for the jacobies of each statement */
    typedef Chunk2< Real, typename IndexHandler::IndexType> JacobiChunk;
    /** @brief The chunk vector for the jacobi data. */
    typedef ChunkVector<JacobiChunk, StatementVector> JacobiVector;

    /** @brief The data for the external functions. */
    typedef Chunk2<ExternalFunction,typename JacobiVector::Position> ExternalFunctionChunk;
    /** @brief The chunk vector for the external  function data. */
    typedef ChunkVector<ExternalFunctionChunk, JacobiVector> ExternalFunctionVector;

    /** @brief The position for all the different data vectors. */
    typedef typename ExternalFunctionVector::Position Position;

    constexpr static const char* tapeName = "ChunkIndexTape";

  };

  /**
   * @brief Helper struct to define the nested vectors for the SimpleIndexTape.
   *
   * See JacobiIndexTape for details.
   */
  template <typename Real, typename IndexHandler>
  struct SimpleIndexTapeTypes {
    typedef Real RealType;
    typedef IndexHandler IndexHandlerType;

    /** @brief The data for each statement. */
    typedef Chunk2<StatementInt, typename IndexHandler::IndexType> StatementChunk;
    /** @brief The chunk vector for the statement data. */
    typedef SingleChunkVector<StatementChunk, EmptyChunkVector> StatementVector;

    /** @brief The data for the jacobies of each statement */
    typedef Chunk2< Real, typename IndexHandler::IndexType> JacobiChunk;
    /** @brief The chunk vector for the jacobi data. */
    typedef SingleChunkVector<JacobiChunk, StatementVector> JacobiVector;

    /** @brief The data for the external functions. */
    typedef Chunk2<ExternalFunction,typename JacobiVector::Position> ExternalFunctionChunk;
    /** @brief The chunk vector for the external  function data. */
    typedef SingleChunkVector<ExternalFunctionChunk, JacobiVector> ExternalFunctionVector;

    /** @brief The position for all the different data vectors. */
    typedef typename ExternalFunctionVector::Position Position;

    constexpr static const char* tapeName = "SimpleIndexTape";

  };

  /**
   * @brief A tape which grows if more space is needed.
   *
   * The JacobiIndexTape implements a fully featured ReverseTapeInterface in a most
   * user friendly fashion. The storage vectors of the tape are grown if the
   * tape runs out of space.
   *
   * This is handled by a nested definition of multiple ChunkVectors which hold
   * the different data vectors. The current implementation uses 3 ChunkVectors
   * and one terminator. The relation is
   *
   * externalFunctions -> jacobiData -> statements
   *
   * The size of the tape can be set with the resize function,
   * the tape will allocate enough chunks such that the given data requirements will fit into the chunks.
   *
   * The tape also uses the index manager IndexHandler to reuse the indices that are deleted.
   * That means that ActiveReal's which use this tape need to be copied by usual means and deleted after
   * the are no longer used. No c-like memory operations like memset and memcpy should be applied
   * to these types.
   *
   * @tparam TapeTypes  All the types for the tape. Including the calculation type and the vector types.
   */
  template <typename TapeTypes>
  class JacobiIndexTape : public ReverseTapeInterface<typename TapeTypes::RealType, typename TapeTypes::IndexHandlerType::IndexType, JacobiIndexTape<TapeTypes>, typename TapeTypes::Position > {
  public:

    typedef typename TapeTypes::RealType Real;
    typedef typename TapeTypes::IndexHandlerType IndexHandler;

    /** @brief The counter for the current expression. */
    EmptyChunkVector emptyVector;

    #define TAPE_NAME JacobiIndexTape

    typedef typename IndexHandler::IndexType IndexType;
    typedef IndexType GradientData;

    #define POSITION_TYPE typename TapeTypes::Position
    #define INDEX_HANDLER_TYPE IndexHandler
    #define RESET_FUNCTION_NAME resetExtFunc
    #define EVALUATE_FUNCTION_NAME evaluateExtFunc
    #include "modules/tapeBaseModule.tpp"

    #define CHILD_VECTOR_TYPE EmptyChunkVector
    #define JACOBI_VECTOR_NAME jacobiVector
    #define VECTOR_TYPE typename TapeTypes::StatementVector
    #define STATEMENT_PUSH_FUNCTION_NAME pushStmtData
    #include "modules/statementModule.tpp"

    #define CHILD_VECTOR_TYPE StmtVector
    #define VECTOR_TYPE typename TapeTypes::JacobiVector
    #include "modules/jacobiModule.tpp"

    #define CHILD_VECTOR_TYPE JacobiVector
    #define CHILD_VECTOR_NAME jacobiVector
    #define VECTOR_TYPE typename TapeTypes::ExternalFunctionVector
    #include "modules/externalFunctionsModule.tpp"

    #undef TAPE_NAME

  public:
    /**
     * @brief Creates a tape with the default chunk sizes for the data, statements and
     * external functions defined in the configuration.
     */
    JacobiIndexTape() :
      emptyVector(),
      indexHandler(),
      adjoints(NULL),
      adjointsSize(0),
      active(false),
      stmtVector(DefaultChunkSize, emptyVector),
      jacobiVector(DefaultChunkSize, stmtVector),
      extFuncVector(1000, jacobiVector) {
    }

    /**
     * @brief Optimization for the copy operation just copies the index of the rhs.
     *
     * No data is stored in this method.
     *
     * The primal value of the lhs is set to the primal value of the rhs.
     *
     * @param[out]   lhsValue    The primal value of the lhs. This value is set to the value
     *                           of the right hand side.
     * @param[out]   lhsIndex    The gradient data of the lhs. The index will be set to the index of the rhs.
     * @param[in]         rhs    The right hand side expression of the assignment.
     */
    inline void store(Real& lhsValue, IndexType& lhsIndex, const ActiveReal<Real, JacobiIndexTape<TapeTypes> >& rhs) {
      ENABLE_CHECK (OptTapeActivity, active){
        ENABLE_CHECK(OptCheckZeroIndex, 0 != rhs.getGradientData()) {
          indexHandler.checkIndex(lhsIndex);

          stmtVector.reserveItems(1); // statements needs a reserve before the data items for the statement are pushed
          jacobiVector.reserveItems(1);
          jacobiVector.setDataAndMove(std::make_tuple(1.0, rhs.getGradientData()));
          stmtVector.setDataAndMove(std::make_tuple((StatementInt)1, lhsIndex));
        } else {
          indexHandler.freeIndex(lhsIndex);
        }
      } else {
        indexHandler.freeIndex(lhsIndex);
      }
      lhsValue = rhs.getValue();
    }

    inline void pushStmtData(const StatementInt& numberOfArguments, const IndexType& lhsIndex) {
      stmtVector.setDataAndMove(std::make_tuple(numberOfArguments, lhsIndex));
    }

    /**
     * @brief Set the size of the jacobi and statement data.
     *
     * The tape will allocate enough chunks such that the given data
     * sizes will fit into the chunk vectors.
     *
     * @param[in]      dataSize  The new size of the jacobi data.
     * @param[in] statementSize  The new size of the statement data.
     */
    void resize(const size_t& dataSize, const size_t& statementSize) {
      resizeJacobi(dataSize);
      resizeStmt(statementSize);
    }

    /**
     * @brief Does nothing because the indices are not connected to the positions.
     *
     * @param[in] start Not used
     * @param[in] end Not used
     */
    inline void clearAdjoints(const Position& start, const Position& end){
      CODI_UNUSED(start);
      CODI_UNUSED(end);
    }

    /**
     * @brief Get the current position of the tape.
     *
     * The position can be used to reset the tape to that position or to
     * evaluate only parts of the tape.
     * @return The current position of the tape.
     */
    inline Position getPosition() const {
      return getExtFuncPosition();
    }

    /**
     * @brief Implementation of the AD stack evaluation.
     *
     * It has to hold startAdjPos >= endAdjPos.
     *
     * @param[inout]           stmtPos The starting point in the expression evaluation. The index is decremented.
     * @param[in]           endStmtPos The ending point in the expression evaluation.
     * @param[in]    numberOfArguments The pointer to the number of arguments of the statement.
     * @param[in]           lhsIndices The pointer the indices of the lhs.
     * @param[inout]           dataPos The current position in the jacobi and index vector. This value is used in the next invocation of this method..
     * @param[in]             jacobies The pointer to the jacobies of the rhs arguments.
     * @param[in]              indices The pointer the indices of the rhs arguments.
     */
    inline void evalStmtCallback(size_t& stmtPos, const size_t& endStmtPos, StatementInt* &numberOfArguments, IndexType* lhsIndices, size_t& dataPos, Real* &jacobies, IndexType* &indices) {

      while(stmtPos > endStmtPos) {
        --stmtPos;
        const IndexType& lhsIndex = lhsIndices[stmtPos];
        const Real adj = adjoints[lhsIndex];
        adjoints[lhsIndex] = 0.0;

        incrementAdjoints(adj, adjoints, numberOfArguments[stmtPos], dataPos, jacobies, indices);
      }
    }

    /**
     * @brief Evaluate a part of the statement vector.
     *
     * It has to hold start >= end.
     *
     * The function calls the evaluation method for the jacobi vector.
     *
     * @param[in] start The starting point for the statement vector.
     * @param[in]   end The ending point for the statement vector.
     */
    template<typename ... Args>
    inline void evaluateStmt(const StmtPosition& start, const StmtPosition& end, Args&&... args) {
      StatementInt* numberOfArgumentsData;
      IndexType* lhsIndexData;

      size_t dataPos = start.data;
      for(size_t curChunk = start.chunk; curChunk > end.chunk; --curChunk) {
        std::tie(numberOfArgumentsData, lhsIndexData) = stmtVector.getDataAtPosition(curChunk, 0);

        evalStmtCallback(dataPos, 0, numberOfArgumentsData, lhsIndexData, std::forward<Args>(args)...);

        dataPos = stmtVector.getChunkUsedData(curChunk - 1);
      }

      // Iterate over the reminder also covers the case if the start chunk and end chunk are the same
      std::tie(numberOfArgumentsData, lhsIndexData) = stmtVector.getDataAtPosition(end.chunk, 0);
      evalStmtCallback(dataPos, end.data, numberOfArgumentsData, lhsIndexData , std::forward<Args>(args)...);
    }


    /**
     * @brief Evaluate a part of the statement vector.
     *
     * It has to hold start >= end.
     *
     * The function calls the evaluation method for the jacobi vector.
     *
     * @param[in] start The starting point for the statement vector.
     * @param[in]   end The ending point for the statement vector.
     */
    inline void evalJacobiesCallback(const StmtPosition& start, const StmtPosition& end, size_t& dataPos, Real* &jacobies, IndexType* &indices) {
      evaluateStmt(start, end, dataPos, jacobies, indices);
    }

    /**
     * @brief Evaluate a part of the statement vector.
     *
     * It has to hold start >= end.
     *
     * The function calls the evaluation method for the jacobi vector.
     *
     * @param[in] start The starting point for the statement vector.
     * @param[in]   end The ending point for the statement vector.
     */
    inline void evalExtFuncCallback(const JacobiPosition& start, const JacobiPosition& end) {
      evaluateJacobies(start, end);
    }

  public:

    /**
     * @brief Register a variable as an active variable.
     *
     * The index of the variable is set to a non zero number.
     * @param[inout] value The value which will be marked as an active variable.
     */
    inline void registerInput(ActiveReal<Real, JacobiIndexTape<TapeTypes> >& value) {
      indexHandler.checkIndex(value.getGradientData());
    }

    /**
     * @brief Not needed in this implementation.
     *
     * @param[in] value Not used.
     */
    inline void registerOutput(ActiveReal<Real, JacobiIndexTape<TapeTypes> >& value) {
      CODI_UNUSED(value);
      /* do nothing */
    }

    /**
     * @brief Prints statistics about the tape on the screen
     *
     * Prints information such as stored statements/adjoints and memory usage on screen.
     */
    void printStatistics() const {

      std::cout << std::endl
                << "-------------------------------------" << std::endl
                << "CoDi Tape Statistics (" << TapeTypes::tapeName << ")" << std::endl
                << "-------------------------------------" << std::endl;
      printTapeBaseStatistics();
      printStmtStatistics();
      printJacobiStatistics();
      printExtFuncStatistics();
      indexHandler.printStatistics();
      std::cout << std::endl;

    }

  };
}
