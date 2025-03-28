#include "LinkedListAPI.h"
#include "VCParser.h"

//Parses and stores information from a vcf file
VCardErrorCode createCard(char* fileName, Card** obj) {

    //Opens file
    FILE *fptr;
    fptr = fopen(fileName, "r");

    //Returns error code
    if (fptr == NULL) {
        return INV_FILE;
    }

    char line[256];
    char prevLine[1024] = {0};
    char lineArray[100][256];
    int row = 0;

    //Allocates memory for the Card object
    *obj = malloc(sizeof(Card));
    if (*obj == NULL) {
        fclose(fptr);
        return OTHER_ERROR;
    }

    //Initializes the card
    (*obj)->fn = NULL;
    (*obj)->optionalProperties = initializeList(propertyToString, deleteProperty, compareProperties);
    (*obj)->birthday = NULL;
    (*obj)->anniversary = NULL;

    //Moves to the end of the file
    fseek(fptr, -1000, SEEK_END);

    char lastLine[1000] = "";

    bool fnFlag = false;

    //Reads until the last line of the file
    while (fgets(line, sizeof(line), fptr)) {
        size_t len = strlen(line);

        //Check fors for valid CLRF endings
        if (len >= 2 && !(line[len - 2] == '\r' && line[len - 1] == '\n')) {
            fclose(fptr);
            //Exits if non valid endings are found
            return INV_CARD;
        }

        line[strcspn(line, "\r\n")] = '\0';

        if (strlen(line) > 0) { 
            strcpy(lastLine, line);
        }

        //Checks for an existing FN property
        if (strncmp(line, "FN", 2) == 0) {
            fnFlag = true;
        }
    }

    //Ensures the file has a valid ending
    if (strcmp(lastLine, "END:VCARD") != 0) {
        fclose(fptr);
        //Exits the program if an invalid ending is found
        return INV_CARD;
    }

    //Ensures there is a FN property
    if (!fnFlag) {
        fclose(fptr);
        return INV_CARD;
    }

    //Rewinds the file pointer to the beginning of the file
    rewind(fptr);

    fgets(line, sizeof(line), fptr);
    line[strcspn(line, "\r\n")] = '\0';

    //Ensures the vCard has a valid beginning
    if (strcmp(line, "BEGIN:VCARD") != 0) {
        fclose(fptr);
        //Exits if an invalid beginning is found
        return INV_CARD;
    }

    fgets(line, sizeof(line), fptr);
    line[strcspn(line, "\r\n")] = '\0';

    //Ensures the version of the vCard is 4.0
    if (strcmp(line, "VERSION:4.0") != 0) {
        fclose(fptr);
        //Exits if an invalid version is entered
        return INV_CARD;
    }

    //Sets file pointer back to the beginning
    rewind(fptr);

    //Reads from file
    while (fgets(line, sizeof(line), fptr)) {

        //Trims newline characters
        line[strcspn(line, "\r\n")] = '\0';

        //Skips begin, version and end
        if (strncmp(line, "BEGIN", 5) == 0 || strncmp(line, "VERSION", 7) == 0 || strncmp(line, "END", 3) == 0) {
            continue;
        }

        //If line begins with a space or tab it needs to be added onto the previous line
        if (line[0] == ' ' || line[0] == '\t') {

            int i = 0;
            int spaceCount = 0;

            //Counts leading whitespaces
            while (line[i] == ' ' || line[i] == '\t') {
                if (line[i] == ' ') {
                    spaceCount++;
                }
                i++;
            }

            //If there were more than one space, preserve whitespace
            if (spaceCount > 1) {
                strcat(prevLine, " ");
            }

            memmove(line, line + i, strlen(line + i) + 1);
            
            strcat(prevLine, line);
        }
        //Stores lines in array
        else {
            //If previous line is not empty, stores it in the array
            if (prevLine[0] != '\0') {
                strncpy(lineArray[row], prevLine, sizeof(lineArray[row]) - 1);
                lineArray[row][sizeof(lineArray[row]) - 1] = '\0';
                row++;

                //Resets previous line for next loop
                prevLine[0] = '\0';
            }

            //Updates the contents of previous line for storing
            strncpy(prevLine, line, sizeof(prevLine) - 1);
            prevLine[sizeof(prevLine) - 1] = '\0';
        }
    }

    //If the final previous line is not empty, stores it in teh array
    if (prevLine[0] != '\0') {
        strncpy(lineArray[row], prevLine, sizeof(lineArray[row]) - 1);
        lineArray[row][sizeof(lineArray[row]) - 1] = '\0';
        row++;
    }

    //Handles parsing and storing of lines
    for (int i = 0; i < row; i++) {

        bool textFlag = 0;
        char *line = lineArray[i];

        //For birthdays
        if (strncmp(line, "BDAY", 4) == 0) {

            //Checks if the birthday is in text format
            char *substringCheck = strstr(line, "text");

            //Sets boolean flag
            if (substringCheck != NULL) {
                textFlag = 1;
            }
        }
    
        //Searches the line for the first :
        char *colonPos = strchr(line, ':');

        if (!colonPos) {
            continue;
        }

        //Replaces the : with a null terminator to split the string
        *colonPos = '\0';

        //Stores the portion after the colon in value
        char *value = colonPos + 1;

        //Checks if the value is empty
        if (value[0] == '\0') {
            fclose(fptr);
            return INV_PROP;
        }

        //Splits property name in token
        char *token = strtok(line, ";");

        if (!token || strlen(token) == 0) {
            fclose(fptr);
            return INV_PROP;
        }

        //Initializes property
        Property *newProp = malloc(sizeof(Property));
        if (newProp == NULL) {
            fclose(fptr);
            return OTHER_ERROR;
        }

        //Initializes parameters and values lists
        newProp->parameters = initializeList(parameterToString, deleteParameter, compareParameters);
        newProp->values = initializeList(valueToString, deleteValue, compareValues);

        //Searches the token for a dot, indicating there is a group
        char *dotPos = strchr(token, '.');
        char *propertyName;

        //If the property has a group
        if (dotPos) {
            //Splits the token into a group and property name
            *dotPos = '\0';

            //Allocates memory for the group
            newProp->group = malloc(strlen(token) + 1);
            if (newProp->group == NULL) {
                deleteProperty(newProp);
                fclose(fptr);
                return OTHER_ERROR;
            }

            //Stores the group name
            strcpy(newProp->group, token);
            propertyName = dotPos + 1;
        }
        //If there is no group 
        else {
            //Allocates memory for the group
            newProp->group = malloc(1);
            if (newProp->group == NULL) {
                deleteProperty(newProp);
                fclose(fptr);
                return OTHER_ERROR;
            }

            //Sets the group to empty string
            strcpy(newProp->group, "");
            propertyName = token;
        }

        //Allocates memory for the property name
        newProp->name = malloc(strlen(propertyName) + 1);
        if (newProp->name == NULL) {
            deleteProperty(newProp);
            fclose(fptr);
            return OTHER_ERROR;
        }

        //Stores the property name
        strcpy(newProp->name, propertyName);

        //Splits the string into parameters
        while ((token = strtok(NULL, ";"))) {

            //Searches string for equals sign
            char *equalSign = strchr(token, '=');

            if (equalSign == NULL || *(equalSign + 1) == '\0') {
                deleteProperty(newProp);
                fclose(fptr);
                return INV_PROP;
            }

            //Splits parameter name and value
            *equalSign = '\0';
            char *paramKey = token;
            char *paramValue = equalSign + 1;


            if (paramKey && paramValue) {
                //Allocates memory for a new parameter
                Parameter *param = malloc(sizeof(Parameter));
                if (param == NULL) {
                    deleteProperty(newProp);
                    fclose(fptr);
                    return OTHER_ERROR;
                }

                //Allocates memory for parameter name
                param->name = malloc(strlen(paramKey) + 1);
                if (param->name == NULL) {
                    free(param);
                    deleteProperty(newProp);
                    fclose(fptr);
                    return OTHER_ERROR;
                }

                //Stores the parameter name
                strcpy(param->name, paramKey);

                //Allocates memory for a value and stores it
                param->value = malloc(strlen(paramValue) + 1);
                if (param->value == NULL) {
                    free(param->name);
                    free(param);
                    deleteProperty(newProp);
                    fclose(fptr);
                    return OTHER_ERROR;
                }

                //Stores the parameter value
                strcpy(param->value, paramValue);

                //Adds parameters to linked list
                insertBack(newProp->parameters, param);
            }
        }

        //Stores multiple values
        if (value) {

            //Allocates memory for a copy of value
            char *tempValue = malloc(strlen(value) + 1);
            if (tempValue == NULL) {
                deleteProperty(newProp);
                fclose(fptr);
                return OTHER_ERROR;
            }

            //Stores a copy of value
            strcpy(tempValue, value);

            char *token1 = tempValue;
            char *token2 = tempValue;
            
            //Loops until the end of value string
            while (*token2) {
                
                //Advances token2 until it reaches a semi-colon or end of line
                while (*token2 && *token2 != ';') {
                    token2++;
                }

                size_t length = token2 - token1;

                //Allocates memory for the value
                char *finalValue = malloc(length + 1);
                if (finalValue == NULL) {
                    deleteProperty(newProp);
                    fclose(fptr);
                    return OTHER_ERROR;
                }

                //Copies the token values into final value
                strncpy(finalValue, token1, length);
                finalValue[length] = '\0';

                //Adds final values into property
                insertBack(newProp->values, finalValue);

                //If reached end of line, break out of loop
                if (!*token2) {
                    break;
                }

                //Moves token
                token1 = ++token2;
            }

            free(tempValue);
        }

        //If property is full name
        if (strcmp(propertyName, "FN") == 0) {
            if ((*obj)->fn != NULL) {
                deleteProperty((*obj)->fn);
            }
            (*obj)->fn = newProp; 
        }
        //If property is birthday or anniversary
        else if (strcmp(propertyName, "BDAY") == 0 || strcmp(propertyName, "ANNIVERSARY") == 0) {

            //Allocates memory for the dates and times
            DateTime *dateField = malloc(sizeof(DateTime));
            if (dateField == NULL) {
                deleteProperty(newProp);
                fclose(fptr);
                return OTHER_ERROR;
            }

            //Sets UTC and isText values to false
            dateField->UTC = false;
            dateField->isText = false;

            //Allocates memory for date
            dateField->date = malloc(9);
            if (dateField->date == NULL) {
                free(dateField);
                deleteProperty(newProp);
                fclose(fptr);
                return OTHER_ERROR;
            }

            int textLength = strlen(value);

            //Allocates memory for time
            dateField->time = malloc(7);
            if (dateField->time == NULL) {
                free(dateField->date);
                free(dateField);
                deleteProperty(newProp);
                fclose(fptr);
                return OTHER_ERROR;
            }

            //Allocates memory for text
            dateField->text = malloc(textLength + 1);
            if (dateField->text == NULL) {
                free(dateField->time);
                free(dateField->date);
                free(dateField);
                deleteProperty(newProp);
                fclose(fptr);
                return OTHER_ERROR;
            }

            //If date is text format
            if (textFlag == 1) {
                strcpy(dateField->text, value);
                dateField->isText = true;
                dateField->date[0] = '\0';
                dateField->time[0] = '\0';
            }
            //If date is not text format
            else {

                //If there is no date specified, store only the time
                if (value[0] == 'T') {
                    dateField->date[0] = '\0';
                    strncpy(dateField->time, value + 1, 6);
                    dateField->time[6] = '\0';
                }
                //If there is a date
                else {
                    char *tempT = strchr(value, 'T');

                    //If there is date and time
                    if (tempT) {
                        //Stores the date
                        size_t dateLength = tempT - value;
                        strncpy(dateField->date, value, dateLength);
                        dateField->date[dateLength] = '\0';

                        //Stores the time
                        strncpy(dateField->time, tempT + 1, 6);
                        dateField->time[6] = '\0';

                        //Sets UTC to true
                        if (tempT[7] == 'Z') {
                            dateField->UTC = true;
                        }
                    }
                    //If there is no time
                    else {
                        //Stores the date
                        strncpy(dateField->date, value, 8);
                        dateField->date[8] = '\0';
                        dateField->time[0] = '\0';
                    }
                }
            }

            //If property is birthday
            if (strcmp(propertyName, "BDAY") == 0) {
                (*obj)->birthday = dateField;
            }
            //If property is anniversary 
            else {
                (*obj)->anniversary = dateField;
            }

            deleteProperty(newProp);
        }
        //Adds to linked list
        else {
            insertBack((*obj)->optionalProperties, newProp);
        }
    }
    fclose(fptr);

    return OK;
}

//Writes the struct to a vcf file
VCardErrorCode writeCard(const char* fileName, const Card* obj) {

    //Checks to see if function parameters are NULL
    if (fileName == NULL || obj == NULL) {
        return WRITE_ERROR;
    }

    //Initializes variables
    char* fullName = NULL;

    //Opens file
    FILE *fptr;
    fptr = fopen(fileName, "w");

    //Returns error code
    if (fptr == NULL) {
        return WRITE_ERROR;
    }

    //Stores full name in a temp variable
    if (obj->fn != NULL && obj->fn->values != NULL && obj->fn->values->head != NULL) {
        fullName = (char*)(obj->fn->values->head->data);
    }

    //A helper function to ensure fprintf writes properly
    int writeCheck(int result) {
        
        //If fprintf returns -1, writing has failed
        if (result < 0) {
            fclose(fptr);
            //Writing failed
            return 1;
        }
        //Writing was successful
        return 0;
    }

    //Writes beginning of file
    if (writeCheck(fprintf(fptr, "BEGIN:VCARD\r\n")) || writeCheck(fprintf(fptr, "VERSION:4.0\r\n"))){
        return WRITE_ERROR;
    }

    //Writes FN property to file
    if (fullName != NULL) {
        if (writeCheck(fprintf(fptr, "FN:%s\r\n", fullName))) {
            return WRITE_ERROR;
        }
    }

    //Writes birthday to file
    if (obj->birthday != NULL) {
        //If birthday stores is in text format
        if (obj->birthday->isText) {
            if (writeCheck(fprintf(fptr, "BDAY;VALUE=text:%s\r\n", obj->birthday->text))) {
                return WRITE_ERROR;
            }
        }
        //If birthday stored has both a date and time
        else if (obj->birthday->date[0] != '\0' && obj->birthday->time[0] != '\0') {
            if(writeCheck(fprintf(fptr, "BDAY:%sT%s\r\n", obj->birthday->date, obj->birthday->time))) {
                return WRITE_ERROR;
            } 
        }
        //If birthday only has a date
        else if (obj->birthday->date[0] != '\0') {
            if (writeCheck(fprintf(fptr, "BDAY:%s\r\n", obj->birthday->date))) {
                return WRITE_ERROR;
            }
        }
        //If birthday only has a time
        else if (obj->birthday->time[0] != '\0') {
            if (writeCheck(fprintf(fptr, "BDAY:T%s\r\n", obj->birthday->time))) {
                return WRITE_ERROR;
            }
        }
    }
    
    //Writes anniversary to file
    if (obj->anniversary != NULL) {
        //If anniversary stored is in text format
        //THIS IS DEF NOT RIGHT THERES NO WAY YOU CAN JUST HARDCODE THIS YOU GODDAMN IDIOT
        if (obj->anniversary->isText) {
            if (writeCheck(fprintf(fptr, "ANNIVERSARY;VALUE=text:%s\r\n", obj->anniversary->text))) {
                return WRITE_ERROR;
            }
        }
        //If anniversary stored has both a date and time
        else if (obj->anniversary->date[0] != '\0' && obj->anniversary->time[0] != '\0') {
            if (writeCheck(fprintf(fptr, "ANNIVERSARY:%sT%s\r\n", obj->anniversary->date, obj->anniversary->time))) {
                return WRITE_ERROR;
            } 
        }
        //If anniversary only has a date
        else if (obj->anniversary->date[0] != '\0') {
            if (writeCheck(fprintf(fptr, "ANNIVERSARY:%s\r\n", obj->anniversary->date))) {
                return WRITE_ERROR;
            }
        }
        //If anniversary only has a time
        else if (obj->anniversary->time[0] != '\0') {
            if (writeCheck(fprintf(fptr, "ANNIVERSARY:T%s\r\n", obj->anniversary->time))) {
                return WRITE_ERROR;
            }
        }
    }

    //Creates an iterator for the properties
    ListIterator propIterator = createIterator(obj->optionalProperties);
    Property *prop;

    //Iterates through existing properties
    while ((prop = nextElement(&propIterator)) != NULL) {

        //Checks for a group and writes to file if one exists
        if (strlen(prop->group) > 0) {
            if (writeCheck(fprintf(fptr, "%s.", prop->group))) {
                return WRITE_ERROR;
            }
        }

        //Writes the name of the property
        if (writeCheck(fprintf(fptr, "%s", prop->name))) {
            return  WRITE_ERROR;
        }

        //Creates an iterator for parameters
        ListIterator paramIterator = createIterator(prop->parameters);
        Parameter *param;

        bool paramCheck = true;

        //Iterates through parameters
        while ((param = nextElement(&paramIterator)) != NULL) {
            if (paramCheck) {
                if (writeCheck(fprintf(fptr, ";"))) {
                    return WRITE_ERROR;
                }
                paramCheck = false;
            }
            else {
                if (writeCheck(fprintf(fptr, ";"))) {
                    return WRITE_ERROR;
                }
            }
            if (writeCheck(fprintf(fptr, "%s=%s", param->name, param->value))) {
                return WRITE_ERROR;
            }
        }

        if (writeCheck(fprintf(fptr, ":"))) {
            return WRITE_ERROR;
        }

        //Creates an iterator for the values
        ListIterator valueIterator = createIterator(prop->values);
        char *value;

        bool valueCheck = true;

        //Iterates through values
        while ((value = nextElement(&valueIterator)) != NULL) {
            if (valueCheck) {
                valueCheck = false;
            }
            else {
                if (writeCheck(fprintf(fptr, ";"))) {
                    return WRITE_ERROR;
                }
            }
            if (writeCheck(fprintf(fptr, "%s", value))) {
                return WRITE_ERROR;
            }
        }

        if (writeCheck(fprintf(fptr, "\r\n"))) {
            return WRITE_ERROR;
        }
    }

    //Writes end of file
    if (writeCheck(fprintf(fptr, "END:VCARD\r\n"))) {
        return WRITE_ERROR;
    }

    fclose(fptr);

    return OK;
}

//Ensures the card object is valid
VCardErrorCode validateCard(const Card* obj) {
    
    //Checks for NULL card object
    if (obj == NULL) {
        return INV_CARD;
    }

    //Checks for NULL fn property
    if (obj->fn == NULL) {
        return INV_CARD;
    }

    //Checks if FN name is NULL or an empty string
    if (obj->fn->name == NULL || strlen(obj->fn->name) == 0) {
        return INV_PROP;
    }

    //Checks if FN values are NULL or an empty string
    if (obj->fn->values == NULL || obj->fn->values->head == NULL) {
        return INV_PROP;
    }

    //Creates an iterator for the FN values list
    ListIterator iterValue = {obj->fn->values->head};

    //Loops through the values of FN to check for NULL values or empty strings
    while (iterValue.current != NULL) {
        char* value = (char*)iterValue.current->data;

        //Checks if any value is NULL or an empty string
        if (value == NULL || strlen(value) == 0) {
            return INV_PROP;
        }

        iterValue.current = iterValue.current->next;
    }

    //Checks if FN group is NULL
    if (obj->fn->group == NULL) {
        return INV_PROP;
    }

    //Checks if FN parameters list is NULL
    if (obj->fn->parameters == NULL) {
        return INV_PROP;
    }

    //Creates an iterator for the FN parameters list
    ListIterator iterParam = {obj->fn->parameters->head};

    //Iterates through the FN parameter list
    while (iterParam.current != NULL) {
        Parameter* param = (Parameter*)iterParam.current->data;

        //Checks if the parameter name is NULL or an empty string
        if (param->name == NULL || strlen(param->name) == 0) {
            return INV_PROP;
        }

        //Checks if the parameter value is NULL or an empty string
        if (param->value == NULL || strlen(param->value) == 0) {
            return INV_PROP;
        }

        iterParam.current = iterParam.current->next;
    }

    //Checks for NULL optional properties list
    if (obj->optionalProperties == NULL) {
        return INV_CARD;
    }

    //Creates an iterator
    ListIterator iterOptProps = {obj->optionalProperties->head};

    //Iterates through the optional properties
    while (iterOptProps.current != NULL) {
        Property* prop = (Property*)iterOptProps.current->data;

        //Checks for a duplicate of VERSION in optional properties
        if (prop != NULL && strcmp(prop->name, "VERSION") == 0) {
            return INV_CARD;
        }

        //Checks if values of the optional property are NULL or an empty strings
        if (prop->values == NULL || prop->values->head == NULL) {
            return INV_PROP;
        }

        //Creates an iterator of the values in optional properties
        ListIterator iterPropValues = {prop->values->head};

        //Iterates through the values of the optional properties
        while (iterPropValues.current != NULL) {
            char* value = (char*)iterPropValues.current->data;

            //Check if the value is NULL or an empty string
            if (value == NULL || (strlen(value) == 0 && strcmp(prop->name, "N") !=0 && strcmp(prop->name, "ADR") != 0)) {
                return INV_PROP;
            }

            iterPropValues.current = iterPropValues.current->next;
        }

        //Checks if the optional property parameters are NULL
        if (prop->parameters != NULL) {

            //Creates an iterator for the parameters of the optional properties
            ListIterator iterPropParam = {prop->parameters->head};

            //Iterates through the parameters or the optional properties
            while (iterPropParam.current != NULL) {
                Parameter* param = (Parameter*)iterPropParam.current->data;

                //Checks if parameter name is NULL or an empty string
                if (param->name == NULL || strlen(param->name) == 0) {
                    return INV_PROP;
                }

                //Checks if the parameter value is NULL or an empty string
                if (param->value == NULL || strlen(param->name) == 0) {
                    return INV_PROP;
                }

                iterPropParam.current = iterPropParam.current->next;
            }
        }

        iterOptProps.current = iterOptProps.current->next;
    }

    //Creates an iterator for the optional properties
    ListIterator iterOptProps2 = {obj->optionalProperties->head};

    //Property name counts
    int kindCount = 0;
    int nCount = 0;
    int genderCount = 0;
    int prodidCount = 0;
    int revCount = 0;
    int uidCount = 0;

    //Iterates through optional properties
    while (iterOptProps2.current != NULL) {
        Property* prop = (Property*)iterOptProps2.current->data;

        //Counts how many properties have the name 'N'
        if (prop != NULL && strcmp(prop->name, "KIND") == 0) {
            kindCount++;

            //Ensure 'KIND' does not appear more than once
            if (kindCount > 1) {
                return INV_PROP;
            }
        }

        //Counts how many properties have the name 'N'
        if (prop != NULL && strcmp(prop->name, "N") == 0) {
            nCount++;

            //Ensure 'N' does not appear more than once
            if (nCount > 1) {
                return INV_PROP;
            }
        }

        //Counts how many properties have the name 'GENDER'
        if (prop != NULL && strcmp(prop->name, "GENDER") == 0) {
            genderCount++;

            //Ensure 'GENDER' does not appear more than once
            if (genderCount > 1) {
                return INV_PROP;
            }
        }

        //Counts how many properties have the name 'PRODID'
        if (prop != NULL && strcmp(prop->name, "PRODID") == 0) {
            prodidCount++;

            //Ensure 'PRODID' does not appear more than once
            if (prodidCount > 1) {
                return INV_PROP;
            }
        }

        //Counts how many properties have the name 'REV'
        if (prop != NULL && strcmp(prop->name, "REV") == 0) {
            revCount++;

            //Ensure 'REV' does not appear more than once
            if (revCount > 1) {
                return INV_PROP;
            }
        }

        //Counts how many properties have the name 'UID'
        if (prop != NULL && strcmp(prop->name, "UID") == 0) {
            uidCount++;

            //Ensure 'UID' does not appear more than once
            if (uidCount > 1) {
                return INV_PROP;
            }
        }

        //Checks if BDAY or ANNIVERSARY exists in optional properties
        if (strcmp(prop->name, "BDAY") == 0 || strcmp(prop->name, "ANNIVERSARY") == 0) {
            return INV_DT;
        }

        iterOptProps2.current = iterOptProps2.current->next;
    }

    //If a birthday exists
    if (obj->birthday != NULL) {
        DateTime* bday = obj->birthday;

        //If the DateTime is a text value
        if (bday->isText) {
            //Ensures date is an empty string
            if (bday->date == NULL || strlen(bday->date) > 0) {
                return INV_DT;
            }
            //Ensures the time is an empty string
            if (bday->time == NULL || strlen(bday->time) > 0) {
                return INV_DT;
            }
            //Ensures UTC is not true
            if (bday->UTC) {
                return INV_DT;
            }
        }

        //If the DateTime is not a text value
        if (!bday->isText) {
            //Ensures text is an empty string
            if (bday->text == NULL || strlen(bday->text) == 0) {
                return INV_DT;
            }

            //Ensures at least date or time is provided
            if ((bday->date == NULL || strlen(bday->date) == 0) && (bday->time == NULL || strlen(bday->time) == 0)) {
                return INV_DT;
            }
        }
    }

    //If an anniversary exists
    if (obj->anniversary != NULL) {
        DateTime* anniv = obj->anniversary;

        //If the DateTime is a text value
        if (anniv->isText) {
            //Ensures date is an empty string
            if (anniv->date == NULL || strlen(anniv->date) == 0) {
                return INV_DT;
            }
            //Ensures the time is an empty string
            if (anniv->time == NULL || strlen(anniv->time) == 0) {
                return INV_DT;
            }
            //Ensures UTC is not true
            if (anniv->UTC) {
                return INV_DT;
            }
        }

        //If the DateTime is not a text value
        if (!anniv->isText) {
            //Ensures text is an empty string
            if (anniv->text == NULL || strlen(anniv->text) == 0) {
                return INV_DT;
            }
        }
    }

    return OK;
}

//Deallocates all memory that was allocated for the card
void deleteCard(Card* obj) {

    if (obj == NULL) {
        return;
    }
    
    //Deallocates FN
    if (obj->fn) {
        deleteProperty(obj->fn);
    }

    //Deallocates birthday
    if (obj->birthday) {
        deleteDate(obj->birthday);
    }

    //Deallocates anniversary
    if (obj->anniversary) {
        deleteDate(obj->anniversary);
    }

    //Deallocates any optional properties
    if (obj->optionalProperties) {
        obj->optionalProperties->deleteData = deleteProperty;
        freeList(obj->optionalProperties);
    }

    //Deallocates card
    free(obj);
}

//Converts the card struct into a readable string
char* cardToString(const Card* obj) {

    if (obj == NULL) {
        return NULL;
    }

    size_t totalSize = 0;

    //Allocates exact memory for the FN name and group
    totalSize += strlen("FN: ") + strlen(obj->fn->name) + strlen(" Group: ") + strlen(obj->fn->group) + 2;

    //Creates an iterator for the parameters
    ListIterator paramIter = createIterator(obj->fn->parameters);
    void *element;

    //Iterates through the parameters
    while ((element = nextElement(&paramIter)) != NULL) {
        //Calls the parameterToString function
        char *paramString = parameterToString(element);
        totalSize += strlen(" ") + strlen(paramString);
        free(paramString);
    }

    //Creates and iterator for the values
    ListIterator valIter = createIterator(obj->fn->values);

    //Iterates through the values
    while ((element = nextElement(&valIter)) != NULL) {
        //Calls the valueToString function
        char *valString = valueToString(element);
        totalSize += strlen(" Value: ") + strlen(valString);
        free(valString);
    }

    //Creates an iterator for the optional properties
    ListIterator optIter = createIterator(obj->optionalProperties);

    //Iterates through the optional properties
    while ((element = nextElement(&optIter)) != NULL) {
        //Calls the propertyToString function
        char *propString = propertyToString(element);
        totalSize += strlen("\n") + strlen(propString);
        free(propString);
    }

    //Calculates the exact memory to allocate for birthday
    if (obj->birthday) {
        char *bdayString = dateToString(obj->birthday);
        totalSize += strlen("\nBirthday: ") + strlen(bdayString);
        free(bdayString);
    }

    //Calculates the exact memory to allocate for anniversary
    if (obj->anniversary) {
        char *annivString = dateToString(obj->anniversary);
        totalSize += strlen("\nAnniversary: ") + strlen(annivString);
        free(annivString);
    }

    //Allocates the exact amount of memory needed for the entire card
    char *cardString = malloc(totalSize + 1);
    if (cardString == NULL) {
        return NULL;
    }

    //Adds FN properties to the string
    snprintf(cardString, totalSize + 1, "FN: %s Group: %s", obj->fn->name, obj->fn->group);

    //Adds parameters to the string
    paramIter = createIterator(obj->fn->parameters);
    while ((element = nextElement(&paramIter)) != NULL) {
        char *paramString = parameterToString(element);
        strcat(cardString, " ");
        strcat(cardString, paramString);
        free(paramString);
    }

    //Adds values to the string
    valIter = createIterator(obj->fn->values);
    while ((element = nextElement(&valIter)) != NULL) {
        char *valString = valueToString(element);
        strcat(cardString, " Value: ");
        strcat(cardString, valString);
        free(valString);
    }

    //Adds optional properties to the string
    optIter = createIterator(obj->optionalProperties);
    while ((element = nextElement(&optIter)) != NULL) {
        char *propString = propertyToString(element);
        strcat(cardString, "\n");
        strcat(cardString, propString);
        free(propString);
    }

    //Adds birthday to the string
    if (obj->birthday) {
        strcat(cardString, "\nBirthday: ");
        char *bdayString = dateToString(obj->birthday);
        strcat(cardString, bdayString);
        free(bdayString);
    }

    //Adds anniversary to the string
    if (obj->anniversary) {
        strcat(cardString, "\nAnniversary: ");
        char *annivString = dateToString(obj->anniversary);
        strcat(cardString, annivString);
        free(annivString);
    }

    //Returns a readable string of the entire card
    return cardString;
}

//Converts error codes into readable strings for the user
char* errorToString(VCardErrorCode err) {

    char *errorText;
    char *result;

    if (err == OK) {
        errorText = "OK";
    }
    else if (err == INV_FILE) {
        errorText = "Invalid file";
    }
    else if (err == INV_CARD) {
        errorText = "Invalid vCard onject";
    }
    else if (err == INV_PROP) {
        errorText = "Invalid property in vCard";
    }
    else if (err == OTHER_ERROR) {
        errorText = "Error unrelated to vCard";
    }
    else if (err == WRITE_ERROR) {
        errorText = "Error writing to file";
    }
    else {
        errorText = "Invalid error code";
    }

    //Allocates memory for the error message
    result = malloc(strlen(errorText) + 1);

    if (result == NULL) {
        return NULL;
    }

    strcpy(result, errorText);

    //Returns the correct error message
    return result;
}
