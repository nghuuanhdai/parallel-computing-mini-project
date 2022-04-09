# Code Documentation

Developers are welcome to supply new functionality to DCDB. In order to keep the documentation consistent they are kindly asked to read and obey the following conventions. The main takeaway is to only write Doxygen-compatible code comments, as DCDB is using [Doxygen](http://www.doxygen.nl/) to generate a fancy HTML documentation out of code comments.

## Conventions

In order to keep all source files consistent there are a few conventions one should follow when developing for DCDB.

### Legal Header

All code source files should have a legal header at the beginning, similar to the following:
```
//================================================================================
// Name        : <Code.c>
// Author      : <Author>
// Copyright   : <Institution or Author>
// Description : <Short description what this file does.>
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) <YYYY> <Institution or Author>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//================================================================================
```
Do not forget to replace all placeholder (indicated by <>) by actual credentials.

### Doxygen Modules <a name="modules"></a>

Doxygen modules are used to sort sources into a navigateable structure, e.g. CollectAgent and Pusher are one module each and each Pusher plugin is a sub-module of the Pusher module. Modules are generated from Doxygen groups in the source code comments. Exemplary code to create a new Doxygen group aka module:
```
/**
 * @defgroup mod Module name
 *
 * @brief Brief one line description of this module.
 *
 * @details Detailed description of this module.
 */
```
Source code parts can be associated to this module by using
```
@ingroup mod
```
in any doxygen comment block. The code documented by this comment will then be associated to the module in the final documentation. The `ingroup` directive can also be used in combination with `defgroup` to create sub-modules.

## Doxygen Cheatsheet

The full reference for writing doxygen-compatible comments can be found at the [website](http://www.doxygen.nl/manual/docblocks.html). As a shortcut, a few documentation examples for the most common code parts are refrained here.

### Classes and Files

Classes should at least be commented with a short description and be sorted into a [module](#modules).
```
/**
 * @brief Brief description.
 *
 * @details More detailed description.
 *
 * @ingroup mod
 */
class DcdbClass { }
```
If there is no class, e.g. a header file with a collection of small utility functions, document the file instead by putting a comment as follows at the beginning.
```
/**
 * @file fileName.h
 *
 * @brief Short file description.
 *
 * @ingroup mod
 */
```

### Methods

One should provide a brief and optionally more detailed description for all methods. All parameters and (if present) the return value should be mentioned in the documenting comment, although a description can be left out if they are self-explanatory. Small unambiguous methods like getters and setters do not have to be documented, see section [Grouping](#grouping) instead.
```
/**
 * @brief One to two line description.
 *
 * @details Optional detailed description, describing intentions,
 *          implementation details and/or special cases.
 *
 * @param count           Description of parameter count.
 * @param selfExplanatory
 *
 * @return True on success, false otherwise. "@return" can be left out for
 *         void type.
 */
bool fun(int count, bool selfExplanatory);
```

### Attributes

All attributes should at least be commented with a short description of their intention.
```
int failCount; ///< Short one line description
```

### Grouping <a name="grouping"></a>
Methods and or attribute within the same class or file which belong together can be grouped. This way they also will be arranged together in the final documentation. A group starts with
```
///@name Few word description
///@{
```
and ends with
```
///@}
```
This is especially usefull for small unambiguous methods like getters/setters. For example:
```
///@name Setters
///@{
void setAtt1(int Att1);
void setAtt2(int Att2);
void setAtt3(int Att3);
///@}

///@name Utility methods
///@{
/**
 * @brief Check internal flag.
 * 
 * @return Status of internal flag.
 */
bool checkFlag();

/**
 * @brief Calculate average of vals.
 *
 * @param vals Vector of integer values.
 *
 * @return Average of all integers in vals.
 */
int  calcAvg(std::vector<int> vals);
///@}
```
