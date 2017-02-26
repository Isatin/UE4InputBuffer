// Copyright 2017 Isaac Hsu. MIT License

#pragma once

/* Holds common static functions for input events. Data members are not allowed here. */
struct FBufferedInputEventKit
{
	/* Returns whether LHS has every bits set as RHS. */
	static bool HasEventFlags(uint64 Input, uint64 Match)
	{
		return (Input & Match) == Match;
	}

	/* Returns whether given bit flags has every bits set as matching one and has no bit set other than the matching and ignoring bits. */
	static bool CompareEventFlags(uint64 Input, uint64 Match, uint64 Ignore)
	{
		uint64 Intersection = Input & Match;
		if (Intersection == Match) // check if the input has every bit set as the matching bits
		{
			uint64 Union = Match | Ignore;
			return (Input | Union) == Union; // return false if the input has any bit set which is not in the matching and ignoring bits
		}
		else
		{
			return false;
		}
	}
};
