#include "xml-parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Easy function to load contents from a file given its path using glibc
char *get_file_content(const char *filepath) {
    FILE *file = fopen(filepath, "r");
    if(file == NULL) {
        fprintf(stderr,"Could open file (%s) to read\n",filepath);
        return NULL;
    }
    
    fseek(file, 0L, SEEK_END);
    long size_file = ftell(file);
    rewind(file);
    
    char *content = malloc(sizeof(char)*(size_file + 1));
    content[size_file] = '\0';
    for (int i = 0; i < size_file; i++) {
        content[i] = fgetc(file);
    }
    
    fclose(file);
    
    return content;
}

// recursive function, could give stackoverflow for really deep nested XML elements
void xml_free_element_recursive(XMLElement* element) {
    if(element == NULL) return;

    free(element->name);
    free(element->text_content);

    XMLAttribute *attr = element->attributes;
    while(attr != NULL) {
        XMLAttribute *next = attr->next;
        free(attr->name);
        free(attr->value);
        free(attr);
        attr = next;
    }

    XMLElement *current_child = element->children;
    while (current_child != NULL) {
        XMLElement *next_sibling = current_child->next_sibling;
        xml_free_element_recursive(current_child);  
        current_child = next_sibling;
    }

    free(element);
}

void xml_free_element(XMLElement* element) {
    xml_free_element_recursive(element); // we use recursion for now
}

void ignore_values(char **cursor, int values[], int values_size) {
    if (cursor == NULL || *cursor == NULL || **cursor == '\0') {
        return;
    }

    if (values_size <= 0) {
        return;
    }

    while(**cursor != '\0') {
        int exist_values = 0;
        
        for(int i = 0; i < values_size; i++) {
            if(**cursor == values[i]) {
                exist_values = 1;
                break;
            }
        }

        if(exist_values) {
            (*cursor)++;
        } else {
            break;
        }
    }
}

// TODO: there could be functions that implement each part of the parsing so it doesnt have that much LOC
// Also it doesnt handle CDATA
XMLElement *parse_xml_element(char **cursor, XMLElement* parent_element) {
    char *current_pos = *cursor;
    
    int skip_values[] = {' ', '\n', '\t', '\r'};

    ignore_values(&current_pos,skip_values,sizeof(skip_values)/sizeof(skip_values[0]));

    if(*current_pos != '<') {
        perror("Error: Expected '<' to start an element.\n");
        *cursor = current_pos; 
        return NULL;
    }

    current_pos++;

    ignore_values(&current_pos,skip_values,sizeof(skip_values)/sizeof(skip_values[0]));

    char *name_start = current_pos;
    while(*current_pos && *current_pos != ' ' && *current_pos != '\t' && *current_pos != '\n' && *current_pos != '\r' && *current_pos != '>' && *current_pos != '/') {
        current_pos++;
    }

    long size_name = current_pos - name_start;
    if (size_name == 0) {
        perror("Error: Element name is empty.\n");
        *cursor = current_pos;
        return NULL;
    }
    
    XMLElement *element = malloc(sizeof(XMLElement));
    if (!element) {
        perror("Malloc failed for XMLElement");
        *cursor = current_pos;
        return NULL;
    }
    
    element->name = NULL;
    element->text_content = NULL;
    element->attributes = NULL;
    element->attributes_size = 0;
    element->parent = parent_element;
    element->children = NULL;
    element->next_sibling = NULL;

    element->name = malloc(size_name + 1);
    if (!element->name) {
        perror("Malloc failed for element name");
        free(element);
        *cursor = current_pos;
        return NULL;
    }

    strncpy(element->name, name_start, size_name);
    element->name[size_name] = '\0';

    while(*current_pos == ' ' || *current_pos == '\n' || *current_pos == '\t' || *current_pos == '\r') {
        current_pos++;
    }

    XMLAttribute *last_attr = NULL;

    while(*current_pos != '/' && *current_pos != '>' && *current_pos != '\0') {
        char *attr_name_start = current_pos;
        while(*current_pos && *current_pos != '=' && *current_pos != ' ' && *current_pos != '\t' && *current_pos != '\n' && *current_pos != '\r') {
            current_pos++;
        }
        long size_attr_name = current_pos - attr_name_start;
        if (size_attr_name == 0) {
            fprintf(stderr, "Error: Attribute name is empty for element %s.\n", element->name);
            *cursor = current_pos;
            xml_free_element(element);
            return NULL;
        }

        char *attr_name_str = malloc(size_attr_name + 1);
        if (!attr_name_str) { xml_free_element(element); *cursor = current_pos; return NULL; }
        strncpy(attr_name_str, attr_name_start, size_attr_name);
        attr_name_str[size_attr_name] = '\0';

        while(*current_pos == ' ' || *current_pos == '\t' || *current_pos == '\n' || *current_pos == '\r') {
            current_pos++;
        }

        if (*current_pos != '=') {
            fprintf(stderr, "Error: Expected '=' after attribute name '%s' for element %s.\n", attr_name_str, element->name);
            free(attr_name_str);
            xml_free_element(element);
            *cursor = current_pos;
            return NULL;
        }

        current_pos++;

        while(*current_pos == ' ' || *current_pos == '\t' || *current_pos == '\n' || *current_pos == '\r') {
            current_pos++;
        }

        if (*current_pos != '\"' && *current_pos != '\'') {
            fprintf(stderr, "Error: Attribute value for '%s' must start with '\"' or \"'\".\n", attr_name_str);
            free(attr_name_str);
            xml_free_element(element);
            *cursor = current_pos;
            return NULL;
        }
        char quote_char = *current_pos;
        current_pos++;

        char *attr_value_start = current_pos;
        while(*current_pos && *current_pos != quote_char) {
            // TODO: handle escaped quotes within the value
            current_pos++;
        }
        if (*current_pos != quote_char) {
            fprintf(stderr, "Error: Attribute value for '%s' not terminated with %c.\n", attr_name_str, quote_char);
            free(attr_name_str);
            xml_free_element(element);
            *cursor = current_pos;
            return NULL;
        }
        long size_attr_value = current_pos - attr_value_start;

        char *attr_value_str = malloc(size_attr_value + 1);
        if (!attr_value_str) { free(attr_name_str); xml_free_element(element); *cursor = current_pos; return NULL; }
        strncpy(attr_value_str, attr_value_start, size_attr_value);
        attr_value_str[size_attr_value] = '\0';

        current_pos++;

        XMLAttribute *new_attr = malloc(sizeof(XMLAttribute));
        if (!new_attr) { xml_free_element(element); free(attr_name_str); free(attr_value_str); *cursor = current_pos; return NULL; }
        new_attr->name = attr_name_str;
        new_attr->value = attr_value_str;
        new_attr->next = NULL;

        if (element->attributes == NULL) {
            element->attributes = new_attr;
        } else {
            last_attr->next = new_attr;
        }
        last_attr = new_attr;
        element->attributes_size++;

        while(*current_pos == ' ' || *current_pos == '\n' || *current_pos == '\t' || *current_pos == '\r') {
            current_pos++;
        }
    }

    if (*current_pos == '/') {
        current_pos++;
        if (*current_pos != '>') {
            fprintf(stderr, "Error: Expected '>' after '/' in self-closing tag for element %s.\n", element->name);
            xml_free_element(element);
            *cursor = current_pos;
            return NULL;
        }
        current_pos++;
        element->text_content = NULL; 
        *cursor = current_pos;
        return element;

    } else if (*current_pos == '>') {
        current_pos++;

        XMLElement *last_child = NULL;
        while (1) {
            char *temp_pos_before_content = current_pos;
            while(*current_pos == ' ' || *current_pos == '\n' || *current_pos == '\t' || *current_pos == '\r') {
                current_pos++;
            }

            if (*current_pos == '\0') {
                fprintf(stderr, "Error: Unexpected end of input while parsing content of %s.\n", element->name);
                xml_free_element(element);
                *cursor = current_pos;
                return NULL;
            }

            if (*current_pos == '<') { 
                if (*(current_pos + 1) == '/') { 
                    current_pos += 2;
                    while(*current_pos == ' ' || *current_pos == '\n' || *current_pos == '\t' || *current_pos == '\r') {
                        current_pos++;
                    }
                    char *closing_name_start = current_pos;
                    while(*current_pos && *current_pos != '>') {
                        current_pos++;
                    }
                    if (*current_pos != '>') {
                        fprintf(stderr, "Error: Closing tag for %s not terminated with '>'.\n", element->name);
                        xml_free_element(element);
                        *cursor = current_pos;
                        return NULL;
                    }
                    long size_closing_name = current_pos - closing_name_start;

                    // Optional: trim trailing whitespace from closing_name_start up to size_closing_name
                    // before strncmp to be more lenient.
                    if (strncmp(element->name, closing_name_start, size_closing_name) == 0 && element->name[size_closing_name] == '\0') {
                        current_pos++;
                        *cursor = current_pos;
                        return element;
                    } else {
                        char temp_closing_name[size_closing_name + 1];
                        strncpy(temp_closing_name, closing_name_start, size_closing_name);
                        temp_closing_name[size_closing_name] = '\0';
                        fprintf(stderr, "Error: Mismatch in closing tag. Expected </%s>, got </%s>.\n", element->name, temp_closing_name);
                        xml_free_element(element);
                        *cursor = current_pos;
                        return NULL; 
                    }
                } else if (*(current_pos + 1) == '!') { 
                    if (strncmp(current_pos, "<!--", 4) == 0) {
                        current_pos += 4;
                        char *comment_end = strstr(current_pos, "-->");
                        if (!comment_end) {
                            perror("Error: Unterminated comment.\n");
                            xml_free_element(element);
                            *cursor = current_pos;
                            return NULL;
                        }
                        current_pos = comment_end + 3; 
                        continue; // go back to look for next child/text/closing tag
                    }
                    else if (strncmp(current_pos, "<![CDATA[", 9) == 0) {
                        current_pos += 9;
                        char *cdata_end = strstr(current_pos, "]]>");
                        if (!cdata_end) {
                            perror("Error: Unterminated CDATA section.\n");
                            xml_free_element(element);
                            *cursor = current_pos;
                            return NULL;
                        }
                        // TODO: capture CDATA as text here.
                        current_pos = cdata_end + 3; 
                        continue;
                    } else {
                        perror("Error: Unsupported XML construct starting with '<!'.\n");
                        xml_free_element(element);
                        *cursor = current_pos;
                        return NULL;
                    }
                } else {
                    XMLElement *child = parse_xml_element(&current_pos, element); // Recursive call
                    if (!child) {
                        fprintf(stderr, "Error parsing child element of %s.\n", element->name);
                        xml_free_element(element);
                        return NULL; 
                    }

                    if (element->children == NULL) {
                        element->children = child;
                    } else {
                        last_child->next_sibling = child;
                    }
                    last_child = child;
                }
            } else { 
                char *text_start = temp_pos_before_content; 
                char *text_end = current_pos; 

                while(*text_end != '<' && *text_end != '\0') {
                    text_end++;
                }

                if (text_end > text_start) {
                    long size_text = text_end - text_start;
                    // TODO: Trim trailing whitespace from text if desired
                    // TODO: Handle XML entities like &, <, etc. in text
                    if (element->text_content) {
                        fprintf(stderr, "Warning: Multiple text nodes or mixed content not fully supported yet, overwriting text for %s.\n", element->name);
                        free(element->text_content);
                    }

                    element->text_content = malloc(size_text + 1);
                    if (!element->text_content) { xml_free_element(element); return NULL; }

                    strncpy(element->text_content, text_start, size_text);
                    element->text_content[size_text] = '\0';
                    current_pos = text_end;
                }
            }
        }
    } else {
        fprintf(stderr, "Error: Expected '>' or '/>' to end tag for element %s, found '%c'.\n", element->name, *current_pos);
        xml_free_element(element);
        *cursor = current_pos;
        return NULL;
    }

    // this should not be reached if logic is correct
    fprintf(stderr, "Error: Unhandled parsing state for element %s.\n", element->name);
    *cursor = current_pos;
    xml_free_element(element);
    return NULL;
}

XMLFile *xml_load(const char *filepath) {
    XMLFile *file = malloc(sizeof(XMLFile));
    if(file == NULL) {
        perror("Could not allocate xml file\n");
        return NULL;
    }
    
    file->version = NULL;
    file->encoding = NULL;
    file->root = NULL;
    
    char *content = get_file_content(filepath);
    if(content == NULL) {
        perror("Could not open file\n");
        free(file);
        return NULL;
    }

    char *current_pos = content;
    if (*current_pos != '<'|| *(current_pos + 1) != '?') {
        perror("Expected <?");
        free(content);
        free(file);
        return NULL;
    }

    current_pos += 2; 

    if (strncmp(current_pos, "xml", 3) != 0) {
        perror("Expected 'xml' after '<?'\n");
        free(content);
        free(file);
        return NULL;
    }

    current_pos += 3;

    char *version_start = strstr(current_pos, "version=\"");
    if (version_start == NULL) {
        perror("XML declaration missing version.\n");
        free(file);
        free(content);
        return NULL;
    }

    version_start += strlen("version=\"");

    char *version_end = strchr(version_start, '\"');
    if (version_end == NULL) {
        perror("Malformed XML declaration: version string not terminated.\n");
        free(file);
        free(content);
        return NULL;
    }

    long size = version_end - version_start;
    if (size <= 0) {
        perror("Malformed XML declaration: version string is missing.\n");
        free(file);
        free(content);
        return NULL;
    }

    file->version = malloc(size + 1);
    if (file->version == NULL) {
        perror("Malloc failed for version\n");
        free(file);
        free(content);
        return NULL;
    }

    strncpy(file->version, version_start, size);
    file->version[size] = '\0';
    current_pos = version_end + 1;

    char *encoding_start = strstr(current_pos, "encoding=\"");
    if (encoding_start == NULL) { 
        perror("Malformed XML declaration: encoding string not started.\n");
        free(content);
        free(file->version);
        free(file);
        return NULL;
    }

    encoding_start += strlen("encoding=\"");

    char *encoding_end = strchr(encoding_start, '\"');
    if (encoding_end == NULL) {
        perror("Malformed XML declaration: encoding string not terminated.\n");
        free(content);
        free(file->version);
        free(file);
        return NULL;
    }

    long encoding_size = encoding_end - encoding_start;
    if (encoding_size <= 0) {
        perror("Malformed XML declaration: encoding is missing.\n");
        free(content);
        free(file->version);
        free(file);
        return NULL;
    }

    file->encoding = malloc(encoding_size + 1);
    if (file->encoding == NULL) {
        perror("Malloc failed for encoding\n");
        free(content);
        free(file->version);
        free(file);

    }

    strncpy(file->encoding, encoding_start, size);
    file->encoding[size] = '\0';
    current_pos = encoding_end + 1;

    char *end = strstr(current_pos, "?>");
    if (end == NULL) {
        perror("Malformed XML declaration: not closed with ?>.\n");
        free(content);
        free(file->version);
        free(file->encoding);
        free(file);
        return NULL;
    }

    current_pos = end + 2;

    file->root = parse_xml_element(&current_pos, NULL);
    free(content);

    return file;
}

void xml_unload(XMLFile *file_struct) {
    if (file_struct == NULL) {
        return;
    }
    free(file_struct->version);
    file_struct->version = NULL;

    free(file_struct->encoding);
    file_struct->encoding = NULL;

    if (file_struct->root != NULL) {
        xml_free_element(file_struct->root);
        file_struct->root = NULL;
    }

    free(file_struct);
    file_struct = NULL;
}

// recursive function, could give stackoverflow for really deep nested XML elements
XMLElement* find_element_by_name_recursive(XMLElement *current_element, const char *tag_name) {
    if (current_element == NULL || tag_name == NULL) {
        return NULL;
    }

    if (current_element->name != NULL && strcmp(current_element->name, tag_name) == 0) {
        return current_element;
    }

    XMLElement *child = current_element->children;
    while (child != NULL) {
        XMLElement *found_in_child = find_element_by_name_recursive(child, tag_name);
        if (found_in_child != NULL) {
            return found_in_child;
        }
        child = child->next_sibling;
    }

    return NULL;
}

XMLElement* xml_element_get_child(XMLElement *start_element, const char *tag_name) {
    if (start_element == NULL || tag_name == NULL) {
        return NULL;
    }

    XMLElement *child = start_element->children;
    while (child != NULL) {
        XMLElement *found = find_element_by_name_recursive(child, tag_name);
        if (found) {
            return found;
        }
        child = child->next_sibling;
    }
    return NULL;
}

XMLAttribute* xml_attribute_get(XMLElement *current_element, const char *attr_name) {
    if (current_element == NULL || attr_name == NULL) {
        return NULL;
    }

    XMLAttribute *child = current_element->attributes;
    while (child != NULL) {
        if(strcmp(child->name, attr_name) == 0) {
            return child;
        }

        child = child->next;
    }

    return NULL;
}

char* xml_attribute_get_value(XMLElement *current_element, const char *attr_name) {
    XMLAttribute* attribute = xml_attribute_get(current_element,attr_name);
    if(attribute != NULL) {
        return attribute->value;
    }
    return NULL;
}