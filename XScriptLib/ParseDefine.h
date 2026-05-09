#pragma once
#include "BaseParse.h"

namespace XScript
{
	class ParseKeyword;
	class ParseDefine :
		public BaseParse
	{
	private:
		std::wstring _define;
		std::vector<const BaseParse*> _list;
		std::vector<std::wstring> _variables;
		std::map<std::wstring, size_t> _variablesMap;

	public:
		ParseDefine(const ParseKeyword* keyword);
		virtual ~ParseDefine();

		void addParse(const BaseParse* parse);
		void addVariable(const ParseKeyword* parse);
		void parseDefine(const ParseDefine* define);

		const std::wstring& define() const;
		std::wstring stringData() const override;
		const std::vector<const BaseParse*>& list() const;
		const std::vector<std::wstring>& variables() const;
	};
}

