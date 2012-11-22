//***************************************************************************
//* Copyright (c) 2012 Saint-Petersburg Academic University
//* All Rights Reserved
//* See file LICENSE for details.
//****************************************************************************

#include "blockfinder.h"

namespace SyntenyFinder
{	
	
	BlockFinder::BlockFinder(const std::vector<FASTARecord> & chrList):
		chrList_(chrList), inRAM_(true)
	{
		Init(chrList);
	}

	BlockFinder::BlockFinder(const std::vector<FASTARecord> & chrList, const std::string & tempDir):
		chrList_(chrList), tempDir_(tempDir), inRAM_(false)
	{
		Init(chrList);
	}

	void BlockFinder::Init(const std::vector<FASTARecord> & chrList)
	{
		originalPos_.resize(chrList.size());
		for(size_t i = 0; i < originalPos_.size(); i++)
		{
			originalPos_[i].resize(chrList[i].sequence.size());
			for(size_t j = 0; j < originalPos_[i].size(); j++)
			{
				originalPos_[i][j] = static_cast<Pos>(j);
			}
		}
	}

	void BlockFinder::ConstructBifStorage(const DNASequence & sequence, const std::vector<std::vector<BifurcationInstance> > & bifurcation, BifurcationStorage & bifStorage) const
	{
		for(size_t strand = 0; strand < 2; strand++)
		{
			size_t nowBif = 0;
			DNASequence::Direction dir = static_cast<DNASequence::Direction>(strand);
			for(size_t chr = 0; chr < sequence.ChrNumber(); chr++)
			{
				size_t pos = 0;
				StrandIterator end = sequence.End(dir, chr);
				for(DNASequence::StrandIterator it = sequence.Begin(dir, chr); it != end; ++it, ++pos)
				{
					if(nowBif < bifurcation[strand].size() && chr == bifurcation[strand][nowBif].chr && pos == bifurcation[strand][nowBif].pos)
					{
						bifStorage.AddPoint(it, bifurcation[strand][nowBif++].bifId);
					}
				}
			}
		}
	}

	void BlockFinder::PerformGraphSimplifications(size_t k, size_t minBranchSize, size_t maxIterations, ProgressCallBack f)
	{
		DNASequence sequence(chrList_, originalPos_, true);
		{
		#ifdef NEW_ENUMERATION		
			size_t maxId;
			std::vector<std::vector<BifurcationInstance> > bifurcation(2);	
			if(inRAM_)
			{
				maxId = EnumerateBifurcationsSArrayInRAM(k, bifurcation[0], bifurcation[1]);
			}
			else
			{
				maxId = EnumerateBifurcationsSArray(k, bifurcation[0], bifurcation[1]);
			}

		#else
			BifurcationStorage bifStorage;
			DNASequence sequence(chrList_, originalPos_);
			EnumerateBifurcationsHash(sequence, bifStorage, k);
		#endif

			BifurcationStorage bifStorage(maxId);
			ConstructBifStorage(sequence, bifurcation, bifStorage);		

		#ifdef _DEBUG
			bifStorage.FormDictionary(idMap, k);
		#endif
			SimplifyGraph(sequence, bifStorage, k, minBranchSize, maxIterations, f);
		}

		for(size_t chr = 0; chr < sequence.ChrNumber(); chr++)
		{
			originalPos_[chr].clear();
			chrList_[chr].sequence.clear();
			StrandIterator end = sequence.PositiveEnd(chr);
			for(StrandIterator it = sequence.PositiveBegin(chr); it != end; ++it)
			{
				chrList_[chr].sequence.push_back(*it);
				originalPos_[chr].push_back(static_cast<Pos>(it.GetOriginalPosition()));
			}
		}
	}	
}