#ifndef XML_PARSER_H
#define XML_PARSER_H

typedef struct XMLAttribute {
    char *name;
    char *value;
    struct XMLAttribute *next;
} XMLAttribute;

typedef struct XMLElement {
    char *name;
    char *text_content;
    
    XMLAttribute *attributes;
    int attributes_size;
    
    struct XMLElement *parent;
    struct XMLElement *children; 
    struct XMLElement *next_sibling;
} XMLElement;

typedef struct XMLFile {
    char *version;
    char *encoding;
    
    XMLElement *root;
} XMLFile;

/**
 * @brief Parse filepath into a XMLFile
 * 
 * @param filepath The path to the XML file to be parsed. Must not be NULL.
 * @return A pointer to a dynamically allocated XMLFile structure representing
 *         the parsed XML document, or NULL if an error occurs (e.g., file
 *         not found, memory allocation failure, or parsing error).
 */

XMLFile *xml_load(const char *filepath);

/**
 * @brief Free the XMLFile struct and all its child XMLElement structs
 * 
 * @param file_struct The given struct to free
 */
void xml_unload(XMLFile *file_struct);

/**
 * @brief search an XMLElement (only children) given its name
 * 
 * @param current_element The structure where the function will start to search its children
 * @param tag_name The name of the tag to search
 * 
 * @return A pointer to a dynamically allocated XMLElement that has the tag_name as name
 *         or NULL if theres isnt a XMLElement that has tag_name as name
 */
XMLElement* xml_element_find_child_by_name(XMLElement *start_element, const char *tag_name);


/**
 * @brief same as xml_element_find_child_by_name but searches the element ITSELF and then its children
 * 
 * @param current_element The structure where the function will start to search its children
 * @param tag_name The name of the tag to search
 * 
 * @return A pointer to a dynamically allocated XMLElement that has the tag_name as name
 *         or NULL if theres isnt a XMLElement that has tag_name as name
 */
XMLElement* xml_element_find_descendant_or_self_by_name(XMLElement *start_element, const char *tag_name);


#endif // XML_PARSER_H