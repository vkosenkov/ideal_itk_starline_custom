#include <grm.h>
#include <epm.h>
#include <aom.h>
#include <aom_prop.h>
#include <item.h>
#include <ps.h>
#include <bom.h>
#include <epm_toolkit_tc_utils.h>
#include <releasestatus.h>
#include <base_utils/ResultCheck.hxx>
#include "../misc.hxx"
#include "attach_used_components.hxx"
#include "../actions/post_remove_all_substitutes.hxx"

tag_t auc_item_revision_type = NULLTAG;

int auc_read_arguments(TC_argument_list_t* arguments, char** statuses_to_ignore)
{
	WRITE_LOG("%s\n", "Reading arguments");
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
		if (arguments_count != 1)
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
			        	if (strcmp("EXCEPT", Flag)==0)
			        	{
			        		if (!STR_EMPTY(normal_value)) {
			        			*statuses_to_ignore = (char *) MEM_alloc(sizeof(char)*(strlen(normal_value)+1));
				                strcpy(*statuses_to_ignore, normal_value);
			        		} else {
			        			*statuses_to_ignore = "";
			        		}
			        	}
			        }
			        if (normal_value != NULL)
			        {
			        	MEM_free(normal_value);
				        normal_value = NULL;
			        }
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

int auc_has_no_except_statuses(tag_t object, int statuses_count, char** statuses, logical* result)
{
	WRITE_LOG("%s\n", "Checking statuses");
	int erc = ITK_ok;
	logical result_r = true;
	char release_status_type[WSO_name_size_c+1];

	try
	{
		int rs_count;
		tag_t* rs_list;
		erc = WSOM_ask_release_status_list(object, &rs_count, &rs_list);
		for(int j = 0; j < statuses_count; j++)
		{
			if(rs_count==0 && strcmp(statuses[j], "NOT")==0) result_r = false;

			for(int i = 0; i < rs_count; i++)
			{
				erc = CR_ask_release_status_type(rs_list[i], release_status_type);
				if(strcmp(release_status_type, statuses[j]) == 0)
				{
					WRITE_LOG("%s\n", "Found exception status");
					result_r = false;
				}
			}
		}
		*result = result_r;
	}
	catch(int exfail)
	{
		return exfail;
	}

	return ITK_ok;
}

int find_components_and_add_them(tag_t root_task, tag_t object, int statuses_count, char** statuses)
{
	WRITE_LOG("%s\n", "Looking for previous revisions");
	int erc = ITK_ok;
	tag_t object_type;
	logical is_type_of;

	try
	{
		erc = TCTYPE_ask_object_type(object, &object_type);
		erc = TCTYPE_is_type_of(object_type, auc_item_revision_type, &is_type_of);

		if(is_type_of)
		{
			WRITE_LOG("%s\n", "Is ItemRevision");

			int bvrs_count = 0;
			tag_t* bvrs = NULL;
			erc = ITEM_rev_list_bom_view_revs(object, &bvrs_count, &bvrs);

			for(int i = 0; i < bvrs_count; i++)
			{
				tag_t bom_window = NULLTAG;
				tag_t top_line = NULLTAG;
				int child_lines_count = 0;
				tag_t* child_lines = NULLTAG;
				tag_t revision = NULLTAG;
				logical result = false;

				erc = get_bom_window_and_top_line_from_bvr(bvrs[i], true, &bom_window, &top_line);
				erc = BOM_line_ask_all_child_lines(top_line, &child_lines_count, &child_lines);
				int number_to_add = 0;
				int* attachments_types_to_add = (int*) MEM_alloc(sizeof(int) * child_lines_count);
				tag_t* attachments_to_add = (tag_t*) MEM_alloc(sizeof(tag_t) * child_lines_count);

				for(int j = 0; j < child_lines_count; j++)
				{
					erc = AOM_ask_value_tag(child_lines[j], "bl_revision", &revision);
					erc = auc_has_no_except_statuses(revision, statuses_count, statuses, &result);
					if(result)
					{
						WRITE_LOG("%s\n", "Adding to attachments");
						try
						{
							attachments_to_add[number_to_add] = revision;
							attachments_types_to_add[number_to_add] = EPM_reference_attachment;
							number_to_add++;
						}
						catch(int exfail)
						{
							WRITE_LOG("%s\n", "Adding to attachments failed");
						}
					}
				}
				erc = BOM_close_window(bom_window);
				erc = EPM_add_attachments(root_task, number_to_add, attachments_to_add, attachments_types_to_add);
				MEM_free(attachments_types_to_add);
				MEM_free(attachments_to_add);

				if(child_lines_count > 0) MEM_free(child_lines);
			}

			if(bvrs_count>0) MEM_free(bvrs);
		}
	}
	catch(int exfail)
	{
		return exfail;
	}

	return ITK_ok;
}

int auc_convert_status_names_string_to_list(char* status_names_string, int* statuses_count, char*** statuses)
{
	WRITE_LOG("%s\n", "Converting status string to list");
	int erc = ITK_ok;
	int status_names_count = 1;
	int count = 0;
	char* delim = ",";
	char *temp = strtok (status_names_string, delim);

	try
	{
		if(strlen(status_names_string) == 0)
		{
			*statuses_count = 0;
			*statuses = NULLTAG;
			return ITK_ok;
		}

		for(int i = 0; i < strlen(status_names_string); i++)
		{
			if(status_names_string[i] == ',') status_names_count++;
		}
		WRITE_LOG("%s\n", "Allocating space for list");
		*statuses = (char**) MEM_alloc(sizeof(statuses) * status_names_count);
		while (temp != NULL)
		{
			WRITE_LOG("%s\n", "Allocating space for list entry");
			*statuses[count] = (char*) MEM_alloc(sizeof(statuses[count]) * (strlen(temp)+1));
			strcpy(*statuses[count++], temp);
		    temp = strtok (NULL, delim);
		}
	}
	catch (int exfail)
	{
		return exfail;
	}

	return ITK_ok;
}

int attach_used_components(EPM_action_message_t msg)
{
	int erc = ITK_ok;
	tag_t
		*attachments,
		root_task;
	int
		*attachments_types,
		attachments_count = 0,
		statuses_count;
	char
		*status_names_to_ignore_string,
		**status_names_to_ignore_list;

	try
	{
		erc = auc_read_arguments(msg.arguments, &status_names_to_ignore_string);
		if(erc!=ITK_ok) throw erc;

		erc = TCTYPE_find_type("ItemRevision", NULL, &auc_item_revision_type);
		erc = auc_convert_status_names_string_to_list(status_names_to_ignore_string, &statuses_count, &status_names_to_ignore_list);

		WRITE_LOG("%s\n", "Asking root task and attachmenmts");
		erc = EPM_ask_root_task(msg.task, &root_task);
		erc = EPM_ask_all_attachments(root_task, &attachments_count, &attachments, &attachments_types);
		for(int i = 0; i < attachments_count; i++)
		{
			if(attachments_types[i]==EPM_target_attachment)
			{
				WRITE_LOG("%s\n", "Working with target");
				find_components_and_add_them(root_task, attachments[i], statuses_count, status_names_to_ignore_list);
			}
		}

		MEM_free(attachments);
		MEM_free(attachments_types);
		for(int i = 0; i < statuses_count; i++)
		{
			MEM_free(status_names_to_ignore_list[i]);
		}
	}
	catch (int exfail)
	{
		return exfail;
	}

	return ITK_ok;
}
