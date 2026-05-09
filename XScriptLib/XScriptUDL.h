#pragma once

namespace XScript
{
	class CScriptData;
	class XScriptUDL
	{
	private:
		CScriptData* _pData;

	public:
		XScriptUDL(CScriptData *data);

		bool writeUDL(const std::wstring& file);
		bool writeAutoComplete(const std::wstring& file);

	private:
		void _writeKeyword(std::ofstream& out, const std::wstring& keyword);
		void _writeKeyword(std::ofstream& out, unsigned int cmd);
	};

}

