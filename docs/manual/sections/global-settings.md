# Configuration Menu

The Configuration Menu is used for project management and to change a variety of software and hardware settings.


![config menu 1](../assets/images/config_menu_1.png)


_The Configuration menu can be opened via the GridPage by pressing the MD's **[Bank Group]**and**[Global]** buttons._


To navigate and enter sub-menus use the **[Up/Down]** buttons and press the **[Enter/Yes]** button. Press the **[Exit/No]** button to go back one level or exit the menu.

## Load Project

The Load Project sub-menu will display a list of MCL Projects that are stored on the root folder of the Micro SD card.


The current project is always selected first and is indicated by an '>' character next to its name.


![project menu](../assets/images/project_menu.png)


### Delete or Rename Project:

From the file options menu, you can delete or rename projects.


From within the Load Project page, press and hold **[Global]** to access the file options menu.

Use the encoder to make your selection, release **[Global]** to activate your choice.


## New Project

The New Project options loads the New Project Page. This page allows you to specify a name for a project which will then be created.


![new project](../assets/images/new_project.png)


![charpane](../assets/images/charpane.png)


_All text editing pages in MCL allow access to char pane. Hold **[Function].**_


## Project Files

Projects are stored on the SD Card in the Projects directory.
Each project has its own folder, inside the project folder there are 3 files:

```text
\Projects\new_project_000\
                          |- new_project_000.mcl   <- Project master file
                          |- new_project_000.0     <- Grid X data
                          |- new_project_000.1     <- Grid Y data
```


## Saving Projects

While loading projects recalls all saved track information, saving projects is not necessary. The project file is updated whenever slots or tracks are saved in the grid.
