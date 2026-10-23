#pragma once
#include "BaseParse.h"
#include "CScriptData.h"
#include <vector>
#include <string>

namespace XScript
{
	// ParseMacro — represents a block macro call in the parse list.
	// Created by _parseMacro when a macro keyword + brackets are detected.
	//
	// The opening { is consumed by _parseMacro (phase 2), after which body
	// lines are drained directly into _items by the top of _parseDataList until
	// the matching } closes the body (depth == 0).  At that point _parseDataList
	// runs _items through itself recursively, then calls _expandMacro to emit
	// the while-loop header.
	class ParseMacro : public BaseParse
	{
	public:
		ParseMacro(const std::wstring& line, const MacroData* macro,
		           std::vector<std::wstring> args,
		           std::vector<const BaseParse*> argNodes,
		           size_t linePos)
			: BaseParse(line, ParseType::Macro)
			, _macro(macro)
			, _args(std::move(args))
			, _argNodes(std::move(argNodes))
			, _macroLinePos(linePos)
			, _isBlock(false)
			, _bodyComplete(false)
			, _depth(0)
		{
		}

		const MacroData*                       macro()         const { return _macro; }
		const std::vector<std::wstring>&       args()          const { return _args; }
		const std::vector<const BaseParse*>&   argNodes()      const { return _argNodes; }
		size_t                                 macroLinePos()  const { return _macroLinePos; }
		bool                                   isBlock()       const { return _isBlock; }
		bool                                   bodyComplete()  const { return _bodyComplete; }
		int                                    depth()         const { return _depth; }

		// Parsed body items — accumulated from _parseDataList outputs
		const std::vector<const BaseParse*>&   items()         const { return _items; }

		void setBlock(bool b)               { _isBlock = b; }
		void setBodyComplete(bool b)        { _bodyComplete = b; }
		void setDepth(int d)                { _depth = d; }
		void addItem(const BaseParse* p)    { _items.push_back(p); }
		void clearItems()                   { _items.clear(); }

	private:
		const MacroData*               _macro;
		std::vector<std::wstring>      _args;
		std::vector<const BaseParse*>  _argNodes;
		size_t                         _macroLinePos;
		bool                           _isBlock;
		bool                           _bodyComplete;
		int                            _depth;
		std::vector<const BaseParse*>  _items; // parsed body — filled by _parseDataList
	};
}
