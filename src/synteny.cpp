//***************************************************************************
//* Copyright (c) 2012 Saint-Petersburg Academic University
//* All Rights Reserved
//* See file LICENSE for details.
//****************************************************************************

#include "blockfinder.h"

namespace SyntenyFinder
{
	std::vector<size_t> BlockFinder::EdgeToVector(const Edge & a)
	{
		size_t feature[] = {a.startVertex, a.endVertex, a.firstChar, a.direction, a.chr};
		return std::vector<size_t>(feature, feature + sizeof(feature) / sizeof(feature[0]));
	}

	bool BlockFinder::EdgeEmpty(const Edge & a, size_t k)
	{
		return a.originalLength < k;
	}

	bool BlockFinder::EdgeCompare(const Edge & a, const Edge & b)
	{
		return EdgeToVector(a) < EdgeToVector(b);
	}

	void BlockFinder::GenerateSyntenyBlocks(size_t k, size_t minSize, std::vector<BlockInstance> & block, bool sharedOnly, ProgressCallBack enumeration)
	{
		block.clear();
	#ifdef NEW_ENUMERATION
		std::vector<std::vector<BifurcationInstance> > bifurcation(2);		
		size_t maxId = EnumerateBifurcationsSArray(k, bifurcation[0], bifurcation[1]);
		BifurcationStorage bifStorage(maxId);
		DNASequence sequence(chrList_, originalPos_);
		ConstructBifStorage(sequence, bifurcation, bifStorage);
	#else
		BifurcationStorage bifStorage;
		DNASequence sequence(chrList_, originalPos_);
		EnumerateBifurcationsHash(sequence, bifStorage, k);
	#endif
		int blockCount = 1;
		block.clear();
		std::vector<Edge> edge;
		std::vector<Edge> nowBlock;
		BlockFinder::ListEdges(sequence, bifStorage, k, edge);
		std::vector<std::set<std::pair<size_t, size_t> > > visit(sequence.ChrNumber());
		edge.erase(std::remove_if(edge.begin(), edge.end(), boost::bind(EdgeEmpty, _1, minSize)), edge.end());
		std::sort(edge.begin(), edge.end(), EdgeCompare);

 		for(size_t now = 0; now < edge.size(); )
		{
			bool hit = false;
			size_t prev = now;
			std::vector<size_t> occur(chrList_.size(), 0);
			for(; now < edge.size() && edge[prev].Coincide(edge[now]); now++)
			{
				occur[edge[now].chr]++;
				std::pair<size_t, size_t> coord(edge[now].originalPosition, edge[now].originalLength);
				hit = hit || (visit[edge[now].chr].count(coord) != 0);
			}

			nowBlock.clear();
			if(!hit && edge[prev].direction == DNASequence::positive && now - prev > 1 && (!sharedOnly || std::count(occur.begin(), occur.end(), 1) == chrList_.size()))
			{
				for(; prev < now; prev++)
				{
					if(std::find_if(nowBlock.begin(), nowBlock.end(), boost::bind(&Edge::Overlap, boost::cref(edge[prev]), _1)) == nowBlock.end())
					{
						nowBlock.push_back(edge[prev]);
					}
				}

				if(nowBlock.size() > 1)
				{					
					for(size_t i = 0; i < nowBlock.size(); i++)
					{
						int strand = nowBlock[i].direction == DNASequence::positive ? +1 : -1;
						visit[nowBlock[i].chr].insert(std::make_pair(nowBlock[i].originalPosition, nowBlock[i].originalLength));
						block.push_back(BlockInstance(blockCount * strand, nowBlock[i].chr, nowBlock[i].originalPosition, nowBlock[i].originalPosition + nowBlock[i].originalLength));
					}

					blockCount++;
				}						
			}
		}

		std::sort(block.begin(), block.end(), CompareBlocksNaturally);
	}
}
