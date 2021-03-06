#include <epm/epm.h>
#include <epm/epm_toolkit_tc_utils.h>
#include <tccore/aom.h>
#include <tccore/grm.h>
#include <tccore/aom_prop.h>
#include <tccore/tctype.h>
#include "../misc.hxx"
#include "assign_form_property_to_targets.hxx"

int read_arguments(TC_argument_list_t* arguments, char** type, char** attribute_name)
{
	int erc = ITK_ok;
	int
		arguments_count = 0;
	char
		*flag = NULL,
		Flag[256],
		*value= NULL,
		*normal_value= NULL;

	try
	{
		arguments_count = TC_number_of_arguments(arguments);
		if (arguments_count != 3)
		{
			throw EPM_wrong_number_of_arguments;
		}

		for (int numctr=0; numctr<arguments_count && erc==ITK_ok;numctr++)
		{
			erc = ITK_ask_argument_named_value(TC_next_argument(arguments), &flag, &value);
			if (erc == ITK_ok)
			{
				erc = EPM_substitute_keyword(value,&normal_value);
			    if (normal_value != NULL)
			    {
			    	StrCpy(Flag,256,flag);
			        StrUpr(Flag);
			        if (erc == ITK_ok)
			        {
			        	if (strcmp("TYPE", Flag)==0)
			        	{
			        		if (!STR_EMPTY(normal_value)) {
			        			*type = (char *) MEM_alloc(sizeof(char)*(strlen(normal_value)+1));
				                strcpy(*type, normal_value);
			        		} else {
			        			*type = "";
			        		}
			        	}
			        	else if(strcmp("ATTRIBUTE", Flag)==0)
			        	{
			        		if (!STR_EMPTY(normal_value)) {
			        			*attribute_name = (char *) MEM_alloc(sizeof(char)*(strlen(normal_value)+1));
			        			strcpy(*attribute_name, normal_value);
			        		} else {
			        			*attribute_name = "";
			        		}
			        	}
			        }
			        MEM_free(normal_value);
				    normal_value = NULL;
			    }
			    else
			    {
			    	WRITE_LOG("%s\n", "Empty attribute");
		   		}
				if (flag != NULL) {
					MEM_free(flag);
					flag = NULL;
				}
				if (value != NULL) {
					MEM_free(value);
					value = NULL;
				}
			}
			else
			{
				WRITE_LOG("Error: rcode1=%i\n", erc);
			}
		}
	}
	catch (int exfail)
	{
		return exfail;
	}

	return ITK_ok;
}

int set_property(tag_t object, char* prop_name, char* prop_value)
{
	int ifail = ITK_ok;

	try
	{
		IFERR_THROW( AOM_refresh(object, TRUE) );
		IFERR_THROW( AOM_set_value_string(object, prop_name, prop_value) );
		IFERR_THROW( AOM_save(object) );
		IFERR_THROW( AOM_refresh(object, FALSE) );
	}
	catch(int exfail)
	{
		ifail = exfail;
	}

	return ifail;
}

int assign_form_property_to_targets(EPM_action_message_t msg)
{
	int ifail = ITK_ok;
	tag_t
		*attachments,
		root_task,
		target_form,
		form_type,
		temp_type;
	int
		*attachments_types,
		attachments_count = 0;
	bool
		is_type_of = false;
	char
		*form_type_name,
		*attribute_name,
		*attribute_value;

	try
	{
		IFERR_THROW( read_arguments(msg.arguments, &form_type_name, &attribute_name) );

		IFERR_THROW( TCTYPE_find_type(form_type_name, NULL, &form_type) );

		IFERR_THROW( EPM_ask_root_task(msg.task, &root_task) );
		IFERR_THROW( EPM_ask_all_attachments(root_task, &attachments_count, &attachments, &attachments_types) );
		for(int i = 0; i < attachments_count; i++)
		{
			if(attachments_types[i]==EPM_target_attachment)
			{
				IFERR_THROW( TCTYPE_ask_object_type(attachments[i], &temp_type) );
				IFERR_THROW( TCTYPE_is_type_of(temp_type, form_type, &is_type_of) );
				if(is_type_of)
				{
					target_form = attachments[i];
					IFERR_THROW( AOM_ask_value_string(target_form, attribute_name, &attribute_value) );
				}
			}
		}

		for(int i = 0; i < attachments_count; i++)
		{
			if(attachments_types[i]==EPM_target_attachment)
			{
				IFERR_THROW( TCTYPE_ask_object_type(attachments[i], &temp_type) );
				set_property(attachments[i], attribute_name, attribute_value);
			}
		}

		MEM_free(attachments);
		MEM_free(attachments_types);
		if(form_type_name!=NULL) MEM_free(form_type_name);
		if(attribute_name!=NULL) MEM_free(attribute_name);
		if(attribute_value!=NULL) MEM_free(attribute_value);
	}
	catch (int exfail)
	{
		ifail = exfail;
	}

	return ifail;
}
