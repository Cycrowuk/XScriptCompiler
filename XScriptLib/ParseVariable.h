#pragma once

#include "BaseParse.h"

namespace XScript
{

	class ParseVariable : public BaseParse
	{
	private:
		DataTypes		_currentDataType;
		std::wstring	_name;
		std::unordered_set<DataTypes> _currentDataTypes;

	public:
		ParseVariable(const std::wstring& l, const std::wstring& name_, const std::unordered_set<DataTypes> *currentType);
		virtual ~ParseVariable();

		void clearDataTypes();
		void addDataType(DataTypes dt);
		void setDataTypes(const std::unordered_set<DataTypes>& types);

		DataTypes dataType() const override;
		DataTypes currentDataType() const;
		std::wstring stringData() const override;
		const std::wstring& name() const;
		const std::unordered_set<DataTypes>& currentDataTypes() const;
	};

}

