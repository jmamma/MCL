/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef PROJECTPAGES_H__
#define PROJECTPAGES_H__

#include "MCLEncoder.h"
#include "MCLFeatureConfig.h"
#include "LoadProjectPage.h"
#ifdef MCL_HAS_PROJECT_BACKUP
#include "ProjectVersionPage.h"
#endif
#ifdef MCL_HAS_PROJECT_CONVERSION
#include "ConvertProjectPage.h"
#endif

extern MCLEncoder loadproj_param1;

extern LoadProjectPage load_proj_page;
#ifdef MCL_HAS_PROJECT_BACKUP
extern ProjectVersionPage project_version_page;
#endif

//extern ConvertProjectPage convert_proj_page;

#endif /* PROJECTPAGES_H__ */
