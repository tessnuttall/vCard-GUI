#include "LinkedListAPI.h"
#include "VCParser.h"

//Deletes a property
void deleteProperty(void* toBeDeleted) {

    if (toBeDeleted == NULL) {
        return;
    }

    //Initializes property
    Property *p = (Property *)toBeDeleted;

    //Frees name and group fields
    free(p->name);
    free(p->group);

    //Frees any parameters
    if (p->parameters) {
        p->parameters->deleteData = deleteParameter;
        freeList(p->parameters);
    }

    //Frees any values
    if (p->values) {
        p->values->deleteData = deleteValue;
        freeList(p->values);
    }

    free(p);
}

int compareProperties(const void* first,const void* second) {

    return 0;
}

//Converts properties to a string
char* propertyToString(void* prop) {

    //Checks to make sure prop is not empty
    if (prop == NULL) {
        char* emptyString = malloc(1);

        if (emptyString) {
            emptyString[0] = '\0';
        }
        return emptyString;
    }

    //Initialises property and calculates its total size for allocation
    Property* property = (Property*)prop;
    size_t totalSize = strlen("Property: ") + strlen(property->name) + strlen(" Group: ") + strlen(property->group) + 2;

    //Creates an iterator for the parameters linked list
    ListIterator paramIter = createIterator(property->parameters);
    void *element;

    //Iterates through the parameters linked list
    while ((element = nextElement(&paramIter)) != NULL) {
        //Converts the current parameter in the linked list to a string
        char *paramString = parameterToString(element);
        //Calculates size
        totalSize += strlen(" ") + strlen(paramString);
        free(paramString);
    }

    //Creates an iterator for the values linked list
    ListIterator valIter = createIterator(property->values);

    //Iterates through the values linked list
    while ((element = nextElement(&valIter)) != NULL) {
        //Converts each value to a string
        char *valString = valueToString(element);
        //Calculates size
        totalSize += strlen(" Value: ") + strlen(valString);
        free(valString);
    }

    //Allocates exact amount of size for property
    char *propString = malloc(totalSize + 1);

    //Stores the property name and group in a string
    snprintf(propString, totalSize + 1, "Property: %s Group: %s", property->name, property->group);

    paramIter = createIterator(property->parameters);

    //Iterates through the parameters
    while ((element = nextElement(&paramIter)) != NULL) {
        //Adds parameter string to the property string
        char *paramString = parameterToString(element);
        strcat(propString, " ");
        strcat(propString, paramString);
        free(paramString);
    }

    valIter = createIterator(property->values);

    //Iterates through the values
    while ((element = nextElement(&valIter)) != NULL) {
        //Adds the values string to the property string
        char *valString = valueToString(element);
        strcat(propString, " Value: ");
        strcat(propString, valString);
        free(valString);
    }

    //Returns the entire property string
    return propString;
}

//Deletes the parameter
void deleteParameter(void* toBeDeleted) {

    if (toBeDeleted == NULL) {
        return;
    }

    //Initializes the parameter
    Parameter *p = (Parameter *)toBeDeleted;

    //Frees the name and value, and parameter
    free(p->name);
    free(p->value);
    free(p);
}

int compareParameters(const void* first,const void* second) {

    return 0;
}

//Converts parameters to a readable string
char* parameterToString(void* param) {

    //Ensures the parameters are not empty
    if (param == NULL) {
        char* emptyString = malloc(1);

        if (emptyString) {
            emptyString[0] = '\0';
        }

        return emptyString;
    }

    //Initializes a parameter
    Parameter* parameter = (Parameter*)param;

    //Calculates exact size needed to allocate
    size_t totalSize = strlen(parameter->name) + strlen("=") + strlen(parameter->value) + 1;

    //Allocates memory for the string
    char *paramString = malloc(totalSize + 1);
    //Stores parameters in the string
    snprintf(paramString, totalSize + 1, "%s=%s", parameter->name, parameter->value);

    //Returns a readable string
    return paramString;
}

//Deletes values
void deleteValue(void* toBeDeleted) {

    if (toBeDeleted != NULL) {
        free(toBeDeleted);
    }
}

int compareValues(const void* first,const void* second) {

    return 0;
}

//Converts values to a readable string
char* valueToString(void* val) {

    //Ensures the value is not empty
    if (val == NULL) {
        char* emptyString = malloc(1);

        if (emptyString) {
            emptyString[0] = '\0';
        }

        return emptyString;
    }

    //Initializes a value and allocates exact memory needed
    char* value = (char*)val;
    size_t size = strlen(value) + 1;

    char* str = malloc(size);
    if (!str) {
        return NULL;
    }

    //Copies the values into the string
    strcpy(str, value);

    //Returns a string of values
    return str;
}

//Deletes the date
void deleteDate(void* toBeDeleted) {

    if (toBeDeleted != NULL) {

        //Initializes date
        DateTime *date = (DateTime *)toBeDeleted;

        //Frees all elements of the date struct
        free(date->date);
        free(date->time);
        free(date->text);
        free(date);
    }
}

int compareDates(const void* first,const void* second) {

    return 0;
}

//Converts the date to a readable string
char* dateToString(void* date) {

    //Ensures the date is not empty
    if (date == NULL) {
        char* emptyString = malloc(1);

        if (emptyString) {
            emptyString[0] = '\0';
        }
        return emptyString;
    }

    //Initializes the date
    DateTime* dt = (DateTime*)date;
    size_t totalSize = 0;

    //Allocates exact memory for text dates
    if (dt->isText) {
        totalSize = strlen(dt->text);
    } 
    //Allocates memory for time dates
    else {
        totalSize = strlen(dt->date) + 1 + strlen(dt->time);
        if (dt->UTC) totalSize += 1; // UTC marker 'Z'
    }

    //Allocates total memory
    char *dateString = malloc(totalSize + 1);

    //Stores text date in a string
    if (dt->isText) {
        snprintf(dateString, totalSize + 1, "%s", dt->text);
    }
    //Stores time dates in a string
    else {
        snprintf(dateString, totalSize + 1, "%sT%s%s", dt->date, dt->time, dt->UTC ? "Z" : "");
    }

    //Returns the date as a readable string
    return dateString;
}