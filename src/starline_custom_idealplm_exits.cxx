#include <iostream>
#include <string>
#include <base_utils/ResultCheck.hxx>
#include <tc/tc.h>
#include <tccore/custom.h>
#include "misc.hxx"
#include "starline_custom_idealplm_exits.hxx"
#include "actions/actions.hxx"
#include "action_handlers/action_handlers.hxx"

//cppcheck-suppress unusedFunction
extern "C" __declspec (dllexport) int starline_custom_idealplm_register_callbacks()
{
	int erc = ITK_ok;

	try
	{
		printf("\Loading idealplm custom library...\n");

		erc = CUSTOM_register_exit("starline_custom_idealplm", "USER_init_module", starline_custom_idealplm_register_actions);
		erc = CUSTOM_register_exit("starline_custom_idealplm", "USER_gs_shell_init_module", starline_custom_idealplm_register_action_handlers);
	}
	catch (...)
	{
		return erc;
	}
	return erc;
}
