#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define READSZ 512
#define LINESZ 512
#define NUMFONTS 5

char bytes[READSZ];
int byteidx = READSZ;

void
maketokens(char* bytes)
{
 char *c = bytes;
 for(; c < bytes + 512 && *c != '\n'; c++){
   if(*c == ' '){
     *c = '\0';
   } 
 } 
 *c = '\0';
}

char**
parsetokens(char* bytes)
{
  char *token = 0;
  char **tokens = (char**)malloc(sizeof(char*)*20);
  int tkindx = 0;
  for(char* c = bytes; c < bytes + 512; c++){
    if (*c != '\0' && !token){
      token = c;
      continue;
    }
    if (token && *c == '\0'){
      tokens[tkindx] = token; 
      tkindx++;
      token = 0; 
    }
  }
  return tokens;
}

int
getline(int fd, char *line)
{
  int lineidx = 0; 
  char c;
  for(;;){
    for(;;){
      if(byteidx >= READSZ){
        break;
      }
      c = bytes[byteidx++];
      line[lineidx++] = c; 
      if (c == '\n'){
        return 1;
      }
    }
    byteidx = 0;
    if(read(fd, bytes, READSZ) == 0){
      return 0;
    }
  }
}

int hexadecimalToDecimal(char hexVal[])
{
    int len = strlen(hexVal);

    // Initializing base value to 1, i.e 16^0
    int base = 1;

    int dec_val = 0;

    // Extracting characters as digits from last character
    for (int i=len-1; i>=0; i--)
    {
        // if character lies in '0'-'9', converting
        // it to integral 0-9 by subtracting 48 from
        // ASCII value.
        if (hexVal[i]>='0' && hexVal[i]<='9')
        {
            dec_val += (hexVal[i] - 48)*base;

            // incrementing base by power
            base = base * 16;
        }

        // if character lies in 'A'-'F' , converting
        // it to integral 10 - 15 by subtracting 55
        // from ASCII value
        else if (hexVal[i]>='A' && hexVal[i]<='F')
        {
            dec_val += (hexVal[i] - 55)*base;

            // incrementing base by power
            base = base*16;
        }
    }

    return dec_val;
}

void
parsefont(uint64 ***fonts, int fd, int fontsize, int fontidx){
  byteidx = READSZ;
  char line[LINESZ];
  uint8 encoding = 0;
  uint64 *bitmap = 0; 
  int bitmapidx = -1;
  char **tokens = 0;
  while(getline(fd, line) != 0){
    maketokens(line); 
    if(tokens){
      free(tokens);
    }
    tokens = parsetokens(line);
    if(!strcmp(tokens[0], "ENCODING")){
      if((encoding = atoi(tokens[1])) >= 255){
        break;
      } 
      bitmap = (uint64*)malloc(sizeof(uint64)*fontsize);
    }
    if(!strcmp(tokens[0] , "BITMAP")){
      bitmapidx = 0;
      continue;
    }
    if(!strcmp(tokens[0], "ENDCHAR")){
      bitmapidx = -1;
      fonts[fontidx][encoding] = bitmap; 
      continue;
    }
    if(bitmapidx >= 0){
     bitmap[bitmapidx++] = hexadecimalToDecimal(tokens[0]); 
     continue;
    }
    if(!strcmp(tokens[0], "ENDFONT")){
      break;
    }
  }
  byteidx = READSZ;
  return;
}

uint64 ***
loadfonts(void){
  printf("start main\n");
  uint64 ***fonts = (uint64***)malloc(sizeof(uint64*)*5);
  int fd;
  for(uint64 ***f = fonts; f < fonts + NUMFONTS; f++){
   *f = (uint64**)malloc(sizeof(uint64*)*256);
  }
  int font_sizes[NUMFONTS] = {12,18,22,28,32};
  char *font_files[NUMFONTS] = {
                    "ter-u12n.bdf",
                    "ter-u18n.bdf", 
                    "ter-u22n.bdf", 
                    "ter-u28n.bdf", 
                    "ter-u32n.bdf"}; 
  for(int i = 0; i < NUMFONTS; i++){
    if((fd = open(font_files[i], O_RDONLY)) < 0){
      printf("no fd\n");
      return 0;
    }; 
    printf("starting font parse for: %s\n", font_files[i]);
    parsefont(fonts, fd, font_sizes[i], i);
    printf("finished font parse for: %s\n", font_files[i]);
    close(fd);
  }
  return fonts;
}
