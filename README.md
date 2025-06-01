# XML-PARSER
Simple XML parser made in plain C language. 
Its very bare bones and has some issues, but it works for me in the types of files im using so...

## Example code:
Reading version and encoding of the XML file (if its specified on the file)
```
#include <stdio.h>
#include <xml-parser.h>

int main() {
  XMLFile *file = xml_load("/path/to/file");
  if(file != NULL) {
    printf("Version: %s\n", file->version);
    printf("Encoding: %s\n", file->encoding);
  } else {
    fprintf(stderr, "Couldnt parse file\n");
    return EXIT_FAILURE;
  }
  
  xml_unload(file);
  
  return EXIT_SUCCESS;
}
```
