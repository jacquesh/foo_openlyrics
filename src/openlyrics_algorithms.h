#pragma once

#include <optional>
#include <vector>

namespace alg
{
    // TODO: Add documentation for these
    // Collapse appropriate contiguous values to produce a shorter sequence where some of the values have been combined.
    // Takes an input vector and an operation by which to collapse values.
    // The operation takes two consecutive values from the sequence.
    // The operation should return `std::pair<T, std::optional<T>>.
    // The first first value in the pair is always used in the resulting sequence.
    // The second value will be ignored if it is None and will be used in the resulting sequence otherwise.
    // When a single value is returned, that value will be the first argument for the following call to `op`.
    // When two values are returned, the first is added to the resulting sequence and the second will be the first argument in the following call to `op`.
    template<typename T, typename TOperation>
    std::vector<T> collapse(const std::vector<T>& input, TOperation op)
    {
        if(input.size() <= 1) return std::vector<T>(input);

        std::vector<T> result;
        T first_arg = *input.begin();
        for(auto iter = ++input.begin(); iter != input.end(); iter++)
        {
            std::pair<T, std::optional<T>> collapsed = op(first_arg, *iter);
            if(collapsed.second.has_value())
            {
                result.emplace_back(std::move(collapsed.first));
                first_arg = std::move(collapsed.second.value());
            }
            else
            {
                first_arg = std::move(collapsed.first);
            }
        }

        result.emplace_back(std::move(first_arg));
        return result;
    }

    // Extends a vector with indices of the values it contains, much like python's `enumerate()`
    // Returns a vector of pairs, each containing first the index of the value and then the value itself
    template<typename TVal>
    std::vector<std::pair<size_t, TVal>> enumerate(std::vector<TVal> input)
    {
        std::vector<std::pair<size_t, TVal>> output;
        output.reserve(input.size());
        size_t index = 0;
        for(TVal& value : input)
        {
            output.push_back({index, std::move(value)});
            index++;
        }
        return output;
    }

    // The inverse of `enumerate`, this takes an enumerated vector and removes the indices,
    // returning just the vector of actual values.
    template<typename TVal>
    std::vector<TVal> denumerate(std::vector<std::pair<size_t, TVal>> input)
    {
        std::vector<TVal> output;
        output.reserve(input.size());
        for(auto& value : input)
        {
            output.push_back(std::move(value.second));
        }
        return output;
    }
}
