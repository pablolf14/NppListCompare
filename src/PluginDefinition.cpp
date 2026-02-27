//this file is part of notepad++
//Copyright (C)2022 Don HO <don.h@free.fr>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "PluginDefinition.h"
#include "menuCmdID.h"
#include <string> 
#include "Scintilla.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>

#define MAXSUPPORTEDLINES_COUNT 1000
#define TextRange Sci_TextRangeFull

std::string newLine = "\r\n";

//
// The plugin data that Notepad++ needs
//
FuncItem funcItem[nbFunc];

//
// The data of Notepad++ that you can use in your plugin commands
//
NppData nppData;

//
// Initialize your plugin data here
// It will be called while plugin loading   
void pluginInit(HANDLE /*hModule*/)
{
}

//
// Here you can do the clean up, save the parameters (if any) for the next session
//
void pluginCleanUp()
{
}

//
// Initialization of your plugin commands
// You should fill your plugins commands here
void commandMenuInit()
{

    //--------------------------------------------//
    //-- STEP 3. CUSTOMIZE YOUR PLUGIN COMMANDS --//
    //--------------------------------------------//
    // with function :
    // setCommand(int index,                      // zero based number to indicate the order of command
    //            TCHAR *commandName,             // the command name that you want to see in plugin menu
    //            PFUNCPLUGINCMD functionPointer, // the symbol of function (function pointer) associated with this command. The body should be defined below. See Step 4.
    //            ShortcutKey *shortcut,          // optional. Define a shortcut to trigger this command
    //            bool check0nInit                // optional. Make this menu item be checked visually
    //            );
    // Shortcut :
    ShortcutKey* shKey = new ShortcutKey;

    shKey = new ShortcutKey;
    shKey->_isAlt = true;
    shKey->_isCtrl = false;
    shKey->_isShift = false;
    shKey->_key = 0x4C; //VK_L
    setCommand(0, TEXT("Compare"), compareLists, shKey, false);

}

//
// Here you can do the clean up (especially for the shortcut)
//
void commandMenuCleanUp()
{
	// Don't forget to deallocate your shortcut here
    delete funcItem[0]._pShKey;
}


//
// This function help you to initialize your plugin commands
//
bool setCommand(size_t index, TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk, bool check0nInit) 
{
    if (index >= nbFunc)
        return false;

    if (!pFunc)
        return false;

    lstrcpy(funcItem[index]._itemName, cmdName);
    funcItem[index]._pFunc = pFunc;
    funcItem[index]._init2Check = check0nInit;
    funcItem[index]._pShKey = sk;

    return true;
}



//----------------------------------------------//
//--         ASSOCIATED FUNCTIONS             --//
//----------------------------------------------//
void showDebugMessage_str(std::string str)
{

    // Convert UTF-8 std::string -> std::wstring
    int wlen = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    std::wstring wline(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wline[0], wlen);

    ::MessageBox(NULL, wline.c_str(), TEXT("Plugin Message"), MB_OK);
}

void readLine(HWND curScint, int lineNumber, std::string& buffer)
{
    // Get start and end positions
    Sci_Position start = (Sci_Position)::SendMessage(curScint, SCI_POSITIONFROMLINE, lineNumber, 0);
    Sci_Position end = (Sci_Position)::SendMessage(curScint, SCI_GETLINEENDPOSITION, lineNumber, 0);

    if (start < 0 || end < start)
        return;

    // Create TextRange
    size_t len = (size_t)(end - start);
    buffer.resize(len + 1, '\0');  // +1 for the null-terminator
    TextRange line;
    line.chrg.cpMin = start;
    line.chrg.cpMax = end;
    line.lpstrText = &buffer[0];


    // Read the line
    ::SendMessage(curScint, SCI_GETTEXTRANGEFULL, 0, (LPARAM)&line);
    return;
}



HWND getCurrentScintilla()
{
    int which = -1;
    SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);

    return (which == 0) ? nppData._scintillaMainHandle
        : nppData._scintillaSecondHandle;
}


void writeFileContentIntoCurrentScintilla_lineByLine(HWND Scintilla, const std::wstring& path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        showDebugMessage_str("ERROR: Could not open the file from writeFileContentIntoCurrentScintilla_lineByLine");
        return;
    }
    
    // Start Sequences of actions to be combined into transactions that are undone as a unit
    ::SendMessage(Scintilla, SCI_BEGINUNDOACTION, 0, 0);

    std::string line;
    std::string linePlusNewLineChar;
    while (std::getline(file, line)) {
        linePlusNewLineChar = line + newLine;
        ::SendMessage(Scintilla, SCI_APPENDTEXT, linePlusNewLineChar.size(), (LPARAM)(linePlusNewLineChar).c_str());
    }
    // Go to the end (-1)
    ::SendMessage(Scintilla, SCI_SETSEL, (WPARAM)-1, (LPARAM)-1);
    
    // End Sequences of actions to be combined into transactions that are undone as a unit
    ::SendMessage(Scintilla, SCI_ENDUNDOACTION, 0, 0);
}


void writeTextIntoCurrentScintilla(HWND Scintilla, std::string text)
{
    // Start Sequences of actions to be combined into transactions that are undone as a unit

    // Write
    std::string linePlusNewLineChar = text + newLine;
    ::SendMessage(Scintilla, SCI_APPENDTEXT, linePlusNewLineChar.size(), (LPARAM)(linePlusNewLineChar).c_str());

    // Go to the end (-1)
    ::SendMessage(Scintilla, SCI_SETSEL, (WPARAM)-1, (LPARAM)-1);
}

void writeTextArrayIntoCurrentScintilla_lineByLine(HWND Scintilla, std::string array[], int arrayLength, bool isolateCTRL_Z)
{
    if (isolateCTRL_Z == true)
    {
        // Start Sequences of actions to be combined into transactions that are undone as a unit
        ::SendMessage(Scintilla, SCI_BEGINUNDOACTION, 0, 0);
    }

    // Write
    std::string linePlusNewLineChar;
    for (int i = 0; i < arrayLength; ++i) {
        linePlusNewLineChar = array[i] + newLine;
        ::SendMessage(Scintilla, SCI_APPENDTEXT, linePlusNewLineChar.size(), (LPARAM)(linePlusNewLineChar).c_str());
    }
    
    // Go to the end (-1)
    ::SendMessage(Scintilla, SCI_SETSEL, (WPARAM)-1, (LPARAM)-1);

    if (isolateCTRL_Z == true)
    {
        // End Sequences of actions to be combined into transactions that are undone as a unit
        ::SendMessage(Scintilla, SCI_ENDUNDOACTION, 0, 0);
    }
}


void sortArray(std::string* v, int lenght, std::string* vSorted) {
    bool sorted[MAXSUPPORTEDLINES_COUNT];
    for (int i = 0; i < lenght; ++i)
        sorted[i] = false;

    for (int pos = 0; pos < lenght; ++pos)
    {
        // Find the candidates to be the next sorted srting
        // Init the 'candidates' array
        bool candidate[MAXSUPPORTEDLINES_COUNT];
        for (int i = 0; i < lenght; ++i)
            candidate[i] = !sorted[i];

        // Find the longest string of the unsorted yet strings
        size_t maxStringLength = 0;
        size_t size = 0;
        for (size_t i = 0; i < lenght; ++i) {
            size = v[i].size();
            if (size > maxStringLength && sorted[i] == false) {
                maxStringLength = size;
            }
        }

        // For each 'c' character of each unsorted-yet strings
        size_t c;
        for (c = 0; c < maxStringLength; ++c) {
            // Get the 'weight' (decimal value of each character)
            int nextCandidateWeight = 9999;

            for (int i = 0; i < lenght; ++i)
            {
                if (candidate[i] == false) continue;

                int weight;
                if (c > v[i].size()) weight = 0;
                else weight = v[i][c];

                if (weight < nextCandidateWeight)
                {
                    // Set the read sting to be a candidate (and reset the previous ones, as the weigth of these is smaller
                    candidate[i] = true; // this is redundant, but put here for clarity
                    nextCandidateWeight = weight;
                    for (int ii = 0; ii < i; ii++)
                    {
                        candidate[ii] = false;
                    }
                }
                else if (weight == nextCandidateWeight)
                    candidate[i] = true; // this is redundant, but put here for clarity
                else if (weight > nextCandidateWeight)
                    candidate[i] = false;

            }
            // Evalueate the numner of candidates
            int numberOfcandidates = 0;
            int chosenCandidate = -1;
            for (int i = 0; i < lenght; ++i) {
                if (candidate[i] == true) {
                    numberOfcandidates += 1;
                    chosenCandidate = i; // only applicable if numberOfcandidates == 1 or sorting two or more equal strings
                }

            }
            if (numberOfcandidates == 1 || numberOfcandidates > 1 && c >= (maxStringLength - 1)) {
                sorted[chosenCandidate] = true;
                vSorted[pos] = v[chosenCandidate];
                break;
            }
        }
    }
}


int removeDuplicates(std::string* v, int lenght, std::string* vWihtoutDuplicates) 
{
    int newArrayLenght = 0;
    for (int i = 0; i < lenght; ++i) {
        std::string candidateToBeAdded = v[i];
        // Find if it is already in the new array
        bool alreadyThere = false;
        for (int j = 0; j < lenght; ++j) {
            if (candidateToBeAdded == vWihtoutDuplicates[j]) {
                alreadyThere = true;
                break;
            }
        }
        if (alreadyThere == false) {
            vWihtoutDuplicates[newArrayLenght] = candidateToBeAdded;
            newArrayLenght += 1;
        }
    }
    return newArrayLenght;
}

void compareLists()
{
    // Read all the lines up to the first empty line. This will be LIST 1
    
    //  Access ORIGINAL DOCUMENT
    HWND sci = getCurrentScintilla();
    
    std::string list1[MAXSUPPORTEDLINES_COUNT];
    std::string list1Sorted[MAXSUPPORTEDLINES_COUNT];
    std::string list1SortedWithoutDuplicates[MAXSUPPORTEDLINES_COUNT];
    int list1Length = 0;
    std::string list2[MAXSUPPORTEDLINES_COUNT];
    std::string list2Sorted[MAXSUPPORTEDLINES_COUNT];
    std::string list2SortedWithoutDuplicates[MAXSUPPORTEDLINES_COUNT];
    int list2Length = 0;
    int lineCount = (int)::SendMessage(sci, SCI_GETLINECOUNT, 0, 0);
    int line_no = 0;
    std::string line;
    
    // Ignore any empty lines at the start
    for (line_no = line_no; line_no < lineCount; line_no++)
    {
        // Read the line
        readLine(sci, line_no, line);
        while (!line.empty() && (line.back() == '\0' || line.back() == '\r' || line.back() == '\n' || line.back() == ' ' || line.back() == '\t'))
            line.pop_back();
        if (!line.empty()) {
            break;
        }
    }


    // ------ Build LIST 2 ------ //
    int i = 0; // To access the list
    for (line_no = line_no; line_no < lineCount && i < MAXSUPPORTEDLINES_COUNT; line_no++)
        {
        // Read the line
        readLine(sci, line_no, line);
        while (!line.empty() && (line.back() == '\0' || line.back() == '\r' || line.back() == '\n' || line.back() == ' ' || line.back() == '\t'))
            line.pop_back();
        if (line.empty()) {
            break;
        }
        else {
            list1[i] = line;
            i++;
        }
    }
    list1Length = i;

    // Sort the list 2 and store in list2Sorted
    sortArray(list1, list1Length, list1Sorted);

    // Ignore any empty lines between the two lists
    for (line_no = line_no; line_no < lineCount; line_no++)
    {
        // Read the line
        readLine(sci, line_no, line);
        while (!line.empty() && (line.back() == '\0' || line.back() == '\r' || line.back() == '\n' || line.back() == ' ' || line.back() == '\t'))
            line.pop_back();
        if (!line.empty()) {
            break;
        }
    }
    
    // ------ Build LIST 2 ------ //
    i = 0; // To access the list
    for (line_no = line_no; line_no < lineCount && i < MAXSUPPORTEDLINES_COUNT; line_no++)
    {
        // Read the line
        readLine(sci, line_no, line);
        while (!line.empty() && (line.back() == '\0' || line.back() == '\r' || line.back() == '\n' || line.back() == ' ' || line.back() == '\t'))
            line.pop_back();
        if (line.empty()) {
            break;
        }
        else {
            list2[i] = line;
            i++;
        }
    }
    list2Length = i;
    
    // Sort the list 2 and store in list2Sorted
    sortArray(list2, list2Length, list2Sorted);


    // Build a structure that defines the numner of appearances of each element across the two lists
    struct ITEM {
        std::string item;
        int appearancesInList1 = 0;
        int appearancesInList2 = 0;
    }items_table[MAXSUPPORTEDLINES_COUNT * 2];
    int items_tableLength = 0;

    // Process the list 1
    for (i = 0; i < list1Length; i++) {
        std::string auxItem = list1Sorted[i];

        // Find if the item is already in the table. In that case, ignore it, as it has been already been counted
        bool alreadyThere = false;
        for (int t = 0; t < items_tableLength; ++t)
        {
            if (auxItem == items_table[t].item)
            {
                alreadyThere = true;
                break;
            }
        }
        if (alreadyThere == true) continue;

        // Add the item to the table
        items_table[items_tableLength].item = auxItem;
        // Count the number of occurrences in the original list 1 (without duplicates removal)
        for (int ii = 0; ii < list1Length; ++ii)
        {
            if (auxItem == list1[ii])
                items_table[items_tableLength].appearancesInList1 += 1;
        }
        // Count the number of occurrences in the original list 2 (without duplicates removal)
        for (int ii = 0; ii < list2Length; ++ii)
        {
            if (auxItem == list2[ii])
                items_table[items_tableLength].appearancesInList2 += 1;
        }
        items_tableLength += 1;
    }
    // Process the list 2
    for (i = 0; i < list2Length; i++) {
        std::string auxItem = list2Sorted[i];

        // Find if the item is already in the table. In that case, ignore it, as it has been already been counted
        bool alreadyThere = false;
        for (int t = 0; t < items_tableLength; ++t)
        {
            if (auxItem == items_table[t].item)
            {
                alreadyThere = true;
                break;
            }
        }
        if (alreadyThere == true) continue;

        // Add the item to the table
        items_table[items_tableLength].item = auxItem;
        // Count the number of occurrences in the original list 1 (without duplicates removal)
        for (int ii = 0; ii < list1Length; ++ii)
        {
            if (auxItem == list1[ii])
                items_table[items_tableLength].appearancesInList1 += 1;
        }
        // Count the number of occurrences in the original list 2 (without duplicates removal)
        for (int ii = 0; ii < list2Length; ++ii)
        {
            if (auxItem == list2[ii])
                items_table[items_tableLength].appearancesInList2 += 1;
        }
        items_tableLength += 1;
    }

    std::string list1Only[MAXSUPPORTEDLINES_COUNT];
    int list1OnlyLength = 0;
    std::string list2Only[MAXSUPPORTEDLINES_COUNT];
    int list2OnlyLength = 0;
    std::string listCommon[MAXSUPPORTEDLINES_COUNT];
    int listCommonLength = 0;


    for (int t = 0; t < items_tableLength; ++t) {
        // Add the item to List 1 if applicable
        if (items_table[t].appearancesInList1 > 0 && items_table[t].appearancesInList2 == 0)
        {
            std::ostringstream oss;
            if (items_table[t].appearancesInList1 == 1) {
                oss << "  " << std::setw(1) << std::setfill('0') << items_table[t].appearancesInList1;
            }
            else {
                oss << " " << std::setw(2) << std::setfill('0') << items_table[t].appearancesInList1;
            }
            oss << "   " << items_table[t].item;

            list1Only[list1OnlyLength] = oss.str();
            list1OnlyLength += 1;
        }
        // Add the item to List 2 if applicable
        if (items_table[t].appearancesInList2 > 0 && items_table[t].appearancesInList1 == 0)
        {
            std::ostringstream oss;
            if (items_table[t].appearancesInList2 == 1) {
                oss << "  " << std::setw(1) << std::setfill('0') << items_table[t].appearancesInList2;
            }
            else {
                oss << " " << std::setw(2) << std::setfill('0') << items_table[t].appearancesInList2;
            }
            oss << "   " << items_table[t].item;

            list2Only[list2OnlyLength] = oss.str();
            list2OnlyLength += 1;
        }
        // Add the item to the Common list if applicable
        if (items_table[t].appearancesInList1 > 0 && items_table[t].appearancesInList2 > 0)
        {
            std::ostringstream oss;
            oss << " ";
            if (items_table[t].appearancesInList1 == 1) {
                oss << " " << std::setw(1) << std::setfill('0') << items_table[t].appearancesInList1 << "  ";
            }
            else {
                oss << ""  << std::setw(2) << std::setfill('0') << items_table[t].appearancesInList1 << "  ";
            }
            if (items_table[t].appearancesInList2 == 1) {
                oss << " " << std::setw(1) << std::setfill('0') << items_table[t].appearancesInList2 << "";
            }
            else {
                oss << ""  << std::setw(2) << std::setfill('0') << items_table[t].appearancesInList2 << "";
            }
            oss << "   " << items_table[t].item;

            listCommon[listCommonLength] = oss.str();
            listCommonLength += 1;
        }
    }

    // ------------- WRITE OUTPUT INTO NOTEPAD++ -------------//
    // Common List
    std::ostringstream oss;
    oss << "\r\n\r\n=====================\r\n"
        << "     COMMMON (" << std::setw(1) << std::setfill('0') << listCommonLength << ")"
        << "\r\n====================="
        << "\r\n L1  L2"
        << "\r\n---------";


    writeTextIntoCurrentScintilla(sci, oss.str());
    writeTextArrayIntoCurrentScintilla_lineByLine(sci, listCommon, listCommonLength, false);

    // List 1 Only
    oss = std::ostringstream{};
    oss << "\r\n\r\n=====================\r\n"
        << "   LIST 1 ONLY (" << std::setw(1) << std::setfill('0') << list1OnlyLength << ")"
        << "\r\n====================="
        << "\r\n  #"
        << "\r\n-----";


    writeTextIntoCurrentScintilla(sci, oss.str());
    writeTextArrayIntoCurrentScintilla_lineByLine(sci, list1Only, list1OnlyLength, false);

    // List 2 Only
    oss = std::ostringstream{};
    oss << "\r\n\r\n=====================\r\n"
        << "   LIST 2 ONLY (" << std::setw(1) << std::setfill('0') << list2OnlyLength << ")"
        << "\r\n====================="
        << "\r\n  #"
        << "\r\n-----";

    writeTextIntoCurrentScintilla(sci, oss.str());
    writeTextArrayIntoCurrentScintilla_lineByLine(sci, list2Only, list2OnlyLength, false);
}



