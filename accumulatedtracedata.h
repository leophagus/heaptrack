/*
 * Copyright 2015 Milian Wolff <mail@milianw.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef ACCUMULATEDTRACEDATA_H
#define ACCUMULATEDTRACEDATA_H

#include <iosfwd>
#include <tuple>
#include <vector>

#include <fstream>
#include <unordered_map>
#include <map>

// sadly, C++ doesn't yet have opaque typedefs
template<typename Base>
struct Index
{
    size_t index = 0;

    explicit operator bool() const
    {
        return index;
    }

    bool operator<(Index o) const
    {
        return index < o.index;
    }

    bool operator!=(Index o) const
    {
        return index != o.index;
    }

    bool operator==(Index o) const
    {
        return index == o.index;
    }
};

struct StringIndex : public Index<StringIndex> {};
struct ModuleIndex : public StringIndex {};
struct FunctionIndex : public StringIndex {};
struct FileIndex : public StringIndex {};
struct IpIndex : public Index<IpIndex> {};
struct TraceIndex : public Index<TraceIndex> {};

struct InstructionPointer
{
    uintptr_t instructionPointer = 0;
    ModuleIndex moduleIndex;
    FunctionIndex functionIndex;
    FileIndex fileIndex;
    int line = 0;

    bool compareWithoutAddress(const InstructionPointer &other) const
    {
        return std::make_tuple(moduleIndex, functionIndex, fileIndex, line)
             < std::make_tuple(other.moduleIndex, other.functionIndex, other.fileIndex, other.line);
    }

    bool equalWithoutAddress(const InstructionPointer &other) const
    {
        return std::make_tuple(moduleIndex, functionIndex, fileIndex, line)
            == std::make_tuple(other.moduleIndex, other.functionIndex, other.fileIndex, other.line);
    }
};

struct TraceNode
{
    IpIndex ipIndex;
    TraceIndex parentIndex;
};

struct AllocationData
{
    // number of allocations
    size_t allocations = 0;
    // bytes allocated in total
    size_t allocated = 0;
    // amount of bytes leaked
    size_t leaked = 0;
    // largest amount of bytes allocated
    size_t peak = 0;
};

struct Allocation : public AllocationData
{
    // backtrace entry point
    TraceIndex traceIndex;
};

/**
 * Merged allocation information by instruction pointer outside of alloc funcs
 */
struct MergedAllocation : public AllocationData
{
    // individual backtraces
    std::vector<Allocation> traces;
    // location
    IpIndex ipIndex;
};

/**
 * Information for a single call to an allocation function
 */
struct AllocationInfo
{
    TraceIndex traceIndex;
    size_t size;
};

struct AccumulatedTraceData
{
    AccumulatedTraceData();
    virtual ~AccumulatedTraceData() = default;

    virtual void handleTimeStamp(size_t newStamp, size_t oldStamp);
    virtual void handleAllocation();
    virtual void handleDebuggee(const char* command);

    void clear();
    const std::string& stringify(const StringIndex stringId) const;

    std::string prettyFunction(const std::string& function) const;

    bool read(const std::string& inputFile);
    bool read(std::istream& in);

    bool shortenTemplates = false;
    bool printHistogram = false;
    bool fromAttached = false;

    std::vector<Allocation> allocations;
    std::map<size_t, size_t> sizeHistogram;
    size_t totalAllocated = 0;
    size_t totalAllocations = 0;
    size_t peak = 0;
    size_t leaked = 0;
    size_t totalTime = 0;
    std::string debuggee;

    // our indices are sequentially increasing thus a new allocation can only ever
    // occur with an index larger than any other we encountered so far
    // this can be used to our advantage in speeding up the findAllocation calls.
    TraceIndex m_maxAllocationTraceIndex;

    Allocation& findAllocation(const TraceIndex traceIndex);

    InstructionPointer findIp(const IpIndex ipIndex) const;

    TraceNode findTrace(const TraceIndex traceIndex) const;

    bool isStopIndex(const StringIndex index) const;

    // indices of functions that should stop the backtrace, e.g. main or static initialization
    std::vector<StringIndex> stopIndices;
    std::unordered_map<uintptr_t, AllocationInfo> activeAllocations;
    std::vector<InstructionPointer> instructionPointers;
    std::vector<TraceNode> traces;
    std::vector<std::string> strings;
};

#endif // ACCUMULATEDTRACEDATA_H
