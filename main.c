/*
** EPC - Easy Prompt Changer
** Copyright (C) 2010 Juan Alberto Regalado Galván <00jarg@gmail.com>
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**  
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**  
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**  
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

#define MAIL "00jarg@gmail.com"
// ERRORS:
// The higher the number the worst the error
#define E_SUCCESS 0
#define E_USAGE   1
#define E_EXISTS  2
#define E_FILE    3


/** ------------------------------- Global Variables -------------------------------- **/

int verbose_flag = 0, i;

// List of the functions for epc
struct list {
  char *function;      // name of the function
  unsigned int defdir; // if defdir = 1 then the function is from DATADIR/functions
  unsigned int used;   // if used = 1 then we have to source that function
  struct list *next;   // pointer to next element
};

// List of the functions for the listing purpouse
struct list2 {
  char *function;
  char *sample;
  struct list2 *next;
};

/** ---------------------------------- Functions ------------------------------------ **/

void exit_error(char *message, int errnum) __attribute__ ((noreturn));
void load_list(struct list **head);
void free_list(struct list **head);
void load_list_sample(struct list2 **head);
void free_list_sample(struct list2 **head);
void new_prompt(char *string);
void save_prompt(char *string);
void change_prompt(char *string);
void list_prompt(void);
void edit_prompt(char *string);
void default_prompt(char *string);
void help(void);

void exit_error(char *message, int errnum) {
  perror(message);
  exit(errnum);
} // end exit_error function

void load_list(struct list **head) {

  struct list *x, *y;
  DIR *dir;
  struct dirent *dirdata;

  // Open the default directory and look for the functions there
  if ((dir = opendir(DATADIR"/functions")) == NULL)
    exit_error(DATADIR"/functions",E_FILE);

  *head = malloc(sizeof(struct list));
  (*head)-> function = NULL;
  (*head)->next = NULL;
  y = *head;

  while ((dirdata = readdir(dir)) != NULL) {
    if (dirdata->d_name[0] != '.') {
      x = malloc(sizeof(struct list));
      x->function = malloc((strlen(dirdata->d_name)+1)*sizeof(char));
      x->function = strcpy(x->function, dirdata->d_name);
      x->used = 0;
      x->defdir = 1;
      x->next = NULL;
      y->next = x;
      y = x;
    }
  }
  closedir(dir);

  char *aux;
  aux = malloc((strlen(getenv("HOME"))+strlen("/.epc/functions")+1)*sizeof(char));
  aux = strcpy(aux, getenv("HOME"));
  aux = strcat(aux, "/.epc/functions");
 
  // Next look for any functions defined by the user in his/her home directory
  if ((dir = opendir(aux)) != NULL) {
    while ((dirdata = readdir(dir)) != NULL) {
      if (dirdata->d_name[0] != '.') {
        x = (*head)->next;
        while (x) {
          if (strcmp(x->function, dirdata->d_name) == 0)
            x->defdir = 0;
          x = x->next;
        }
      }
    }
  } 

  closedir(dir);
  free(aux);

} // end load_list function

void free_list(struct list **head) {

  struct list *x, *y;

  x = *head;
  y = x->next;
  while (x) {
    free(x->function);
    x->next = NULL;
    free(x);
    x = y;
    if (x) y = x->next;  
  }

} // end free_list function

void load_list_sample(struct list2 **head) {

  struct list2 *x, *y;
  DIR *dir;
  struct dirent *dirdata;
  char *aux;
  FILE *fd;
  int size = 0;

  // Open the default directory and look for the functions there
  if ((dir = opendir(DATADIR"/functions")) == NULL)
    exit_error(DATADIR"/functions",E_FILE);

  *head = malloc(sizeof(struct list));
  (*head)->function = NULL;
  (*head)->sample = NULL; 
  (*head)->next = NULL;
  y = *head;

  while ((dirdata = readdir(dir)) != NULL) {
    if (dirdata->d_name[0] != '.') {
      x = malloc(sizeof(struct list));
      x->function = malloc((strlen(dirdata->d_name)+1)*sizeof(char));
      x->function = strcpy(x->function, dirdata->d_name);
      aux = malloc((strlen(DATADIR"/functions/")+strlen(x->function)+1)*sizeof(char));
      aux = strcpy(aux, DATADIR"/functions/");
      aux = strcat(aux, x->function);
      fd = fopen(aux, "rt"); free(aux); aux = NULL;
      while (!feof(fd)) {
        getline(&aux, &size, fd);
        aux[strlen(aux)-1] = '\0';
        if (aux[0] == '#' && aux[1] == '#') {
          x->sample = malloc((strlen(aux+3)+1)*sizeof(char));
          x->sample = strcpy(x->sample, aux+3);
          break;
        }
      }
      fclose(fd);
      if (!x->sample) {
        x->sample = malloc((strlen("No preview")+1)*sizeof(char));
        x->sample = strcpy(x->sample, "No preview");
      }
      x->next = NULL;
      y->next = x;
      y = x;
    }
  }
  closedir(dir);

  free(aux);
  aux = malloc((strlen(getenv("HOME"))+strlen("/.epc/functions")+1)*sizeof(char));
  aux = strcpy(aux, getenv("HOME"));
  aux = strcat(aux, "/.epc/functions");
 
  // Next look for any functions defined by the user in his/her home directory
  int match;
  if ((dir = opendir(aux)) != NULL) {
    while ((dirdata = readdir(dir)) != NULL) {
      match = 0;
      if (dirdata->d_name[0] != '.') {
        x = (*head)->next; y = x;
        while (x) {
          if (strcmp(dirdata->d_name, x->function) == 0) {
            free(x->sample);
            match = 1;
            break;
          }
          y = x;
          x = x->next;
        }
        if (!match) {
          x = malloc(sizeof(struct list));
          x->function = malloc((strlen(dirdata->d_name)+1)*sizeof(char));
          x->function = strcpy(x->function, dirdata->d_name);
          y->next = x;
        }
        aux = malloc((strlen(getenv("HOME"))+strlen("/.epc/functions")+strlen(x->function)+1)*sizeof(char));
        aux = strcpy(aux, getenv("HOME"));
        aux = strcat(aux, "/.epc/functions");
        aux = strcat(aux, x->function);
        fd = fopen(aux, "rt"); free(aux); aux = NULL;
        while (!feof(fd)) {
          getline(&aux, &size, fd);
          aux[strlen(aux)-1] = '\0';
          if (aux[0] == '#' && aux[1] == '#') {
            x->sample = malloc((strlen(aux+3)+1)*sizeof(char));
            x->sample = strcpy(x->sample, aux+3);
            break;
          }
        }
      }
    }
  } 

  closedir(dir);
  free(aux);

} // end load_list_sample

void free_list_sample(struct list2 **head) {

  struct list2 *x, *y;

  x = *head;
  y = x->next;
  while (x) {
    free(x->function);
    free(x->sample);
    x->next = NULL;
    free(x);
    x = y;
    if (x) y = x->next;  
  }

} // end free_list_sample function

void new_prompt(char *string) {

  struct list *head, *x = NULL;
  DIR *dir;

  load_list(&head);

  char *aux;
  aux = malloc((strlen(getenv("HOME"))+strlen("/.epc")+1)*sizeof(char));
  aux = strcpy(aux, getenv("HOME"));
  aux = strcat(aux, "/.epc");

  // If the .epc directory doesn't exist inside the home directory
  // create it. And create a temporary file for epcf to source
  if ((dir = opendir(aux)) == NULL)
    if (mkdir(aux, 0755))
      exit_error(aux, E_FILE);
  closedir(dir);  

  FILE *fd;

  free(aux);
  aux = malloc((strlen(getenv("HOME"))+strlen("/.epc/tmp")+1)*sizeof(char));
  aux = strcpy(aux, getenv("HOME"));
  aux = strcat(aux, "/.epc/tmp");
  fd = fopen(aux, "w+t");

  fprintf(fd, "#!/bin/bash\nexport PS1=\"");

  // Now parse the string to construct the prompt
  free(aux); aux = NULL;

  unsigned int match;

  int size = 0, quote = 0; i = 0;
  while ((string[i] == ' ') && (string[i+1] != '\0'))
    i++;
  while (string[i] != '\0') {
    match = 0;
    if (((string[i] == ' ') && (!quote)) || (string[i+1] == '\0')) {
      if ((string[i+1] == '\0') && (string[i] != ' ')) {
        // If we are in the last character (and it's not a space) then copy it to aux, next analize the string
        aux = (char *)realloc(aux, (++size)*sizeof(char));
        aux[size-1] = string[i];
        aux[size] = '\0';
      }
      // Compare the token with the list of functions
      x = head->next;
      while (x) {
        if (strcmp(x->function,aux) == 0) {
          fprintf(fd, "\\$(%s)", aux);
          x->used = 1;
          match = 1;
          break;
        }
        x = x->next;
      }
      // Next compare the token with the corresponding bash's prompt especial characters
      // Note that this are overriden by the functions above
      if (!match) {
        if (strcmp(aux,"bell") == 0) {
          fprintf(fd, "\\a");
          match = 1;
        } else if (strcmp(aux,"date") == 0) {
          fprintf(fd, "\\D{");
          i++; while ((string[i] == ' ') && (string[i] != '\0')) i++;
          while ((string[i] != '\0') && (string[i] != ' ')) {
            fprintf(fd, "%c", string[i]);
            i++;
          }
          fprintf(fd,"}");
          match = 1;
          if (string[i] == '\0') break;
        } else if (strcmp(aux,"hostname") == 0) {
          fprintf(fd, "\\h");
          match = 1;
        } else if (strcmp(aux,"hostfull") == 0) {
          fprintf(fd, "\\H");
          match = 1;
        } else if (strcmp(aux,"jobs") == 0) {
          fprintf(fd, "\\j");
          match = 1;
        } else if (strcmp(aux,"terminal") == 0) {
          fprintf(fd, "\\l");
          match = 1;
        } else if (strcmp(aux,"newline") == 0) {
          fprintf(fd, "\\n");
          match = 1;
        } else if (strcmp(aux,"creturn") == 0) {
          fprintf(fd, "\\r");
          match = 1;
        } else if (strcmp(aux,"shell") == 0) {
          fprintf(fd, "\\s");
          match = 1;
        } else if (strcmp(aux,"user") == 0) {
          fprintf(fd, "\\u");
          match = 1;
        } else if (strcmp(aux,"version") == 0) {
          fprintf(fd, "\\v");
          match = 1;
        } else if (strcmp(aux,"release") == 0) {
          fprintf(fd, "\\V");
          match = 1;
        } else if (strcmp(aux,"wdir") == 0) {
          fprintf(fd, "\\w");
          match = 1;
        } else if (strcmp(aux,"Wdir") == 0) {
          fprintf(fd, "\\W");
          match = 1;
        } else if (strcmp(aux,"hisnum") == 0) {
          fprintf(fd, "\\!");
          match = 1;
        } else if (strcmp(aux,"cmdnum") == 0) {
          fprintf(fd, "\\#");
          match = 1;
        } else if (strcmp(aux,"exit") == 0) {
          fprintf(fd, "\\$?");
          match = 1;
        } else if (strcmp(aux,"sign") == 0) {
          fprintf(fd, "\\\\$");
          match = 1;
        } else if (strcmp(aux,"\\") == 0) {
          fprintf(fd, "\\\\");
          match = 1;
        } else if (strcmp(aux,"space") == 0) {
          fprintf(fd, " ");
          match = 1;
        } else if (strcmp(aux,"quote") == 0) {
          fprintf(fd, "\\\"");
          match = 1;
        // Colors:
        // Foreground
        } else if (strcmp(aux,"black") == 0) {
          fprintf(fd, "\\[\\e[30m\\]");
          match = 1;
        } else if (strcmp(aux,"red") == 0) {
          fprintf(fd, "\\[\\e[31m\\]");
          match = 1;
        } else if (strcmp(aux,"green") == 0) {
          fprintf(fd, "\\[\\e[32m\\]");
          match = 1;
        } else if (strcmp(aux,"yellow") == 0) {
          fprintf(fd, "\\[\\e[33m\\]");
          match = 1;
        } else if (strcmp(aux,"blue") == 0) {
          fprintf(fd, "\\[\\e[34m\\]");
          match = 1;
        } else if (strcmp(aux,"magenta") == 0) {
          fprintf(fd, "\\[\\e[35m\\]");
          match = 1;
        } else if (strcmp(aux,"cyan") == 0) {
          fprintf(fd, "\\[\\e[36m\\]");
          match = 1;
        } else if (strcmp(aux,"white") == 0) {
          fprintf(fd, "\\[\\e[37m\\]");
          match = 1;
        // Background
        } else if (strcmp(aux,"bblack") == 0) {
          fprintf(fd, "\\[\\e[40m\\]");
          match = 1;
        } else if (strcmp(aux,"bred") == 0) {
          fprintf(fd, "\\[\\e[41m\\]");
          match = 1;
        } else if (strcmp(aux,"bgreen") == 0) {
          fprintf(fd, "\\[\\e[42m\\]");
          match = 1;
        } else if (strcmp(aux,"byellow") == 0) {
          fprintf(fd, "\\[\\e[43m\\]");
          match = 1;
        } else if (strcmp(aux,"bblue") == 0) {
          fprintf(fd, "\\[\\e[44m\\]");
          match = 1;
        } else if (strcmp(aux,"bmagenta") == 0) {
          fprintf(fd, "\\[\\e[45m\\]");
          match = 1;
        } else if (strcmp(aux,"bcyan") == 0) {
          fprintf(fd, "\\[\\e[46m\\]");
          match = 1;
        } else if (strcmp(aux,"bwhite") == 0) {
          fprintf(fd, "\\[\\e[47m\\]");
          match = 1;
        // Text attributes
        } else if (strcmp(aux,"reset") == 0) {
          fprintf(fd, "\\[\\e[0m\\]");
          match = 1;
        } else if (strcmp(aux,"bright") == 0) {
          fprintf(fd, "\\[\\e[1m\\]");
          match = 1;
        } else if (strcmp(aux,"dim") == 0) {
          fprintf(fd, "\\[\\e[2m\\]");
          match = 1;
        } else if (strcmp(aux,"underline") == 0) {
          fprintf(fd, "\\[\\e[4m\\]");
          match = 1;
        } else if (strcmp(aux,"blink") == 0) {
          fprintf(fd, "\\[\\e[5m\\]");
          match = 1;
        } else if (strcmp(aux,"reverse") == 0) {
          fprintf(fd, "\\[\\e[7m\\]");
          match = 1;
        } else if (strcmp(aux,"hidden") == 0) {
          fprintf(fd, "\\[\\e[8m\\]");
          match = 1;
        }
      }
      // If is not a token use it literaly
      if (!match) fprintf(fd, "%s", aux);
      // Then cut all the remaining spaces
      while ((string[i+1] == ' ') && (string[i+1] != '\0'))
       i++;
      free(aux); size = 0; aux = NULL;
    } else if (string[i] == '"') {
      // If there's something whitin double quotes then copy that as it is
      i++;
      while(string[i] != '"') {
        fprintf(fd, "%c", string[i]);
        i++;
        // We reach the end and no quote is found
        if (string[i] == '\0') {
          free(aux);
          aux = malloc((strlen(getenv("HOME"))+strlen("/.epc/tmp")+1)*sizeof(char));
          aux = strcpy(aux, getenv("HOME"));
          aux = strcat(aux, "/.epc/tmp");
          unlink(aux);
          free_list(&head);
          puts("Error: Quote unmatched");
          exit(E_USAGE);
        }
      }
      free(aux); size = 0; aux = NULL; quote = 1;
      if (string[i+1] == ' ') i++;
    } else { 
      // Keep copying the character into aux
      aux = (char *)realloc(aux, (++size)*sizeof(char));
      aux[size-1] = string[i];
      aux[size] = '\0';
      quote = 0;
    }
    i++;
  }

  fprintf(fd, "\"\n"
  "{ cd $HOME/.epc\n"
  "if [ -f loadedf ]; then\n"
  "  while read func; do\n"
  "    unset $func\n"
  "  done <loadedf\n"
  "  rm loadedf\n"
  "fi } 2> /dev/null\n");
 
  x = head->next;
  while (x) {
    if (x->used)
      if (x->defdir)
        fprintf(fd, "source "DATADIR"/functions/%s\n", x->function);
      else
        fprintf(fd, "source $HOME/.epc/functions/%s\n", x->function);
    x = x->next;
  }

  fprintf(fd, "cat >> loadedf <<\"EOF\"\n");

  x = head->next;
  while (x) {
    if (x->used)
      fprintf(fd, "%s\n", x->function);
    x = x->next;
  }

  fprintf(fd, "EOF\n"
  "cd - > /dev/null 2>&1\n");

  free_list(&head);
  fclose(fd);
  exit(E_SUCCESS);

} // end new_prompt function

void save_prompt(char *string) {

  char *aux, *line = NULL;
  int size = 0, isnumber = 1;
  FILE *fd;

  aux = malloc((strlen(getenv("HOME"))+strlen("/.epc/prompts")+1)*sizeof(char));
  aux = strcpy(aux, getenv("HOME"));
  aux = strcat(aux, "/.epc/prompts");
  

  // First check that the name is not already in use
  fd = fopen(aux, "a+t");
  while (!feof(fd)) {
    getline(&line, &size, fd);
    // Cut the new line from the line
    line[strlen(line)-1] = '\0';
    if (strcmp(line, string) == 0) {
      printf("Prompt with the name \"%s\" already exists\n", string);
      puts("Please specify another name");
      puts("The prompt will not be saved");
      free(aux); free(line);
      fclose(fd);
      exit(E_EXISTS);
    }
    // Skip the other line
    getline(&line, &size, fd);
    fscanf(fd, "\n");
  }
  // Then check that the name is not a number
  for (i = 0; i < strlen(string); i++)
    if (!isdigit(string[i])) {
      isnumber = 0;
      break;
    }
  if (isnumber) {
    puts("The prompt name could not be a number");
    puts("Please specify a valid name");
    puts("The prompt will not be saved");
    free(aux); free(line);
    fclose(fd);
    exit(E_USAGE);
  }

  // Write the name first in one line, then in the next line the prompt
  fprintf(fd, "%s\n", string);
  free(aux);
  aux = malloc((strlen(getenv("PS1"))+1)*sizeof(char));
  aux = strcpy(aux, getenv("PS1"));
  // Hold on, before we need to scape the functions or they won't be actualized in the prompt
  i = 0;
  while (aux[i] !='\0') {
    if (aux[i] == '$') fprintf(fd, "\\");
    fprintf(fd, "%c", aux[i]);
    i++;
  }

  fprintf(fd, "\n", aux);

  fclose(fd);
  free(aux);
  free(line);

  if (verbose_flag) printf("Prompt %s successfully saved\n", string);
  exit(E_SUCCESS);

} // end save_prompt function

void change_prompt(char *string) {

  char *aux, *line = NULL;
  int num = 0, isnumber = 1, size = 0, match = 0;
  FILE *fd;

  // First check if the argument passed is a number
  for (i = 0; i < strlen(string); i++)
    if (!isdigit(string[i])) {
      isnumber = 0;
      break;
    }

  aux = malloc((strlen(getenv("HOME"))+strlen("/.epc/prompts")+1)*sizeof(char));
  aux = strcpy(aux, getenv("HOME"));
  aux = strcat(aux, "/.epc/prompts");
  
  fd = fopen(aux, "rt");
  if (fd == NULL) {
    puts("Your prompt database is empty");
    free(aux);
    exit(E_FILE);
  }

  if (isnumber) {
    num = atoi(string);
    i = 1;
    while (!feof(fd)) {
      getline(&line, &size, fd);
      getline(&line, &size, fd);
      fscanf(fd, "\n");
      if (i == num) { 
        match = 1;
        break;
      }
      i++;
    }
    if (!match) {
      printf("Prompt number %d does not exist\n", num);
      free(aux); free(line); fclose(fd);
      exit(E_EXISTS);
    }
  } else {
    while (!feof(fd)) {
      getline(&line, &size, fd);
      // Cut the new line from the line
      line[strlen(line)-1] = '\0';
      if (strcmp(string, line) == 0) {
        match = 1;
        break;
      }
      getline(&line, &size, fd);
      fscanf(fd, "\n");
    }
    getline(&line, &size, fd);
    fscanf(fd, "\n");
    if (!match) {
      printf("Prompt with name \"%s\" does not exist\n", string);
      free(aux); free(line); fclose(fd);
      exit(E_EXISTS);  
    }
  }
  
  fclose (fd); free(aux);
  aux = malloc((strlen(getenv("HOME"))+strlen("/.epc/tmp")+1)*sizeof(char));
  aux = strcpy(aux, getenv("HOME"));
  aux = strcat(aux, "/.epc/tmp");

  // Open the temporary file to change the prompt
  fd = fopen(aux, "w+t");
  line[strlen(line)-1] = '\0';
  fprintf(fd, "#!/bin/bash\nexport PS1=\"%s\"\n", line); 
  // And load all the functions that prompt uses
  // First unset the previously loaded if any
  fprintf(fd, 
  "{ cd $HOME/.epc\n"
  "if [ -f loadedf ]; then\n"
  "  while read func; do\n"
  "    unset $func\n"
  "  done <loadedf\n"
  "  rm loadedf\n"
  "fi } 2> /dev/null\n");

  struct list *head, *x;
  load_list(&head);
  free(aux); aux = NULL;

  // Analize the prompt and mark the functions that need to be loaded
  i = 0;
  while (line[i] != '\0') {
    if (line[i] == '$' && line[i+1] == '(') {
      i += 2; free(aux); aux = NULL; size = 0;
      while (line[i] != ')') {
        aux = (char *)realloc(aux, (++size)*sizeof(char));
        aux[size-1] = line[i];
        aux[size] = '\0';
        i++;
      }
      // Compare the token with the list of functions
      x = head->next;
      while (x) {
        if (strcmp(x->function,aux) == 0) {
          x->used = 1;
          break;
        }
        x = x->next;
      }
    }
    i++;
  }

  x = head->next;
  while (x) {
    if (x->used)
      if (x->defdir)
        fprintf(fd, "source "DATADIR"/functions/%s\n", x->function);
      else
        fprintf(fd, "source $HOME/.epc/functions/%s\n", x->function);
    x = x->next;
  }

  fprintf(fd, "cat >> loadedf <<\"EOF\"\n");

  x = head->next;
  while (x) {
    if (x->used)
      fprintf(fd, "%s\n", x->function);
    x = x->next;
  }

  fprintf(fd, "EOF\n"
  "cd - > /dev/null 2>&1\n");

  free_list(&head);
  free(line);
  fclose(fd);

  if (verbose_flag) {
    if (isnumber) printf("Prompt changed to prompt %d\n", num);
    else printf("Prompt changed to prompt \"%s\"\n", string);
  }

  exit(E_SUCCESS);  

} // end change_prompt function

void list_prompt(void) {

  char *aux, *line = NULL;
  FILE *fd;
  int size = 0, j, sizep;

  aux = malloc((strlen(getenv("HOME"))+strlen("/.epc/prompts")+1)*sizeof(char));
  aux = strcpy(aux, getenv("HOME"));
  aux = strcat(aux, "/.epc/prompts");
  
  fd = fopen(aux, "rt");
  if (fd == NULL) {
    puts("Your prompt database is empty");
    free(aux);
    exit(E_FILE);
  }

  struct list2 *head, *x;
  load_list_sample(&head);

  printf("\tPrompt name\t\tPrompt\n");
  printf("\t-----------\t\t------\n");
  i = 1;
  while (!feof(fd)) {
    printf("%d\t",i);
    getline(&line, &size, fd);
    line[strlen(line)-1] = '\0';
    printf("%s\t\t", line);
    getline(&line, &size, fd);
    // Convert the string prompt into a preview of the prompt
    j = 0;
    while (line[j] !='\n') {
      switch (line[j]) {
        case '\\':
          j++;
          switch (line[j]) {
            case '[':
              while (!(line[j] == '\\' && line[j+1] == ']') && line[j] != '\n') {
                j++;
                free(aux); aux = (char *)malloc(4*sizeof(char));
                aux[0] = line[j]; aux[1] = line[j+1]; aux[2] = line[j+2]; aux[3] = '\0';
                if (strcmp(aux,"\\e[") == 0) {
                  j+= 3; free(aux); aux = NULL; sizep = 0;
                  while (line[j] != 'm' && line[j] != '\0') {
                    aux = (char *)realloc(aux, (++sizep)*sizeof(char));
                    aux[sizep-1] = line[j];
                    aux[sizep] = '\0';
                    j++;
                  }
                  printf("%c[%sm", 0x1B, aux);
                  break;
                } else
                    printf("%c", line[j]);
              }
              j += 2;
              break;
            case '$':
              if (line[j+1] == '?') {
                printf("0"); j++;
              } else if (line[j+1] == '(') {
                j += 2;
                free(aux); aux = NULL; sizep = 0;
                while (line[j] != ')' && line[j] != '\0') {
                  aux = (char *)realloc(aux, (++sizep)*sizeof(char));
                  aux[sizep-1] = line[j];
                  aux[sizep] = '\0';
                  j++;
                }
                x = head->next;
                while (x) {
                  if (strcmp(x->function, aux) == 0) {
                    printf("%s", x->sample);
                    break;
                  }
                  x = x->next;
                }
              }
              break;
            case '\\':
              if (line[j+1] != '$') printf("\\");
              else { printf("$"); j++; }
              break;
            case 'a':
              printf("beep");
              break;
            case 'D':
              while (line[j] != '}' && line[j] != '\n') {
                j++;
              }
              printf("31/05/1989 07:00");
              break;
            case 'h':
              printf("hostname");
              break;
            case 'H':
              printf("hostnamefull");
              break;
            case 'j':
              printf("1");
              break;
            case 'l':
              printf("tty1");
              break;
            case 'n':
              printf("\n\t\t\t");
              break;
            case 'r':
              printf("\r");
              break;
            case 's':
              printf("bash");
              break;
            case 'u':
              printf("user");
              break;
            case 'v':
              printf("4.1");
              break;
            case 'V':
              printf("4.1.7");
              break;
            case 'w':
              printf("/some/dir");
              break;
            case 'W':
              printf("dir");
              break;
            case '!':
              printf("1049");
              break;
            case '#':
              printf("50");
              break;
          }
          break;
        default : 
          printf("%c", line[j]);
          break;
      }
      j++;
    }
    printf("\n");
    fscanf(fd, "\n");
    i++;
  }

  free_list_sample(&head);
  exit(E_SUCCESS);

} // end list_prompt function

void edit_prompt(char *string) {

  char *aux = NULL, *line = NULL;
  int num = 0, isnumber = 1, size = 0, match = 0, sizep = 0;
  FILE *fd;
  struct list *head, *x;

  if (string) {
    // First check if the argument passed is a number
    for (i = 0; i < strlen(string); i++)
      if (!isdigit(string[i])) {
        isnumber = 0;
        break;
      }

    aux = malloc((strlen(getenv("HOME"))+strlen("/.epc/prompts")+1)*sizeof(char));
    aux = strcpy(aux, getenv("HOME"));
    aux = strcat(aux, "/.epc/prompts");
  
    fd = fopen(aux, "rt");
    if (fd == NULL) {
      puts("Your prompt database is empty");
      free(aux);
      exit(E_FILE);
    }

    if (isnumber) {
      num = atoi(string);
      i = 1;
      while (!feof(fd)) {
        getline(&line, &size, fd);
        getline(&line, &size, fd);
        fscanf(fd, "\n");
        if (i == num) { 
          match = 1;
          break;
        }
        i++;
      }
      if (!match) {
        printf("Prompt number %d does not exist\n", num);
        free(aux); free(line); fclose(fd);
        exit(E_EXISTS);
      }
    } else {
      while (!feof(fd)) {
        getline(&line, &size, fd);
        // Cut the new line from the line
        line[strlen(line)-1] = '\0';
        if (strcmp(string, line) == 0) {
          match = 1;
          break;
        }
        getline(&line, &size, fd);
        fscanf(fd, "\n");
      }
      getline(&line, &size, fd);
      fscanf(fd, "\n");
      if (!match) {
        printf("Prompt with name \"%s\" does not exist\n", string);
        free(aux); free(line); fclose(fd);
        exit(E_EXISTS);  
      }
    }
    line[strlen(line)-1] = '\0';
    fclose(fd); free(aux);
  } else { // Edit the current prompt
    line = malloc((strlen(getenv("PS1"))+1)*sizeof(char));
    line = strcpy(line, getenv("PS1"));
  }

  printf("epc -n '");
  i = 0; load_list(&head);
  while(line[i] != '\0') {
    printf(" ");
    switch (line[i]) {
      case '\\':
        i++;
        switch (line[i]) {
          case '[':
              while (!(line[i] == '\\' && line[i+1] == ']') && line[i] != '\n') {
                i++;
                free(aux); aux = (char *)malloc(4*sizeof(char));
                aux[0] = line[i]; aux[1] = line[i+1]; aux[2] = line[i+2]; aux[3] = '\0';
                if (strcmp(aux,"\\e[") == 0) {
                  i += 3; 
                  while (line[i] != 'm' && line[i] != '\0') {
                    free(aux); aux = NULL; sizep = 0;
                    while (line[i] != ';' && line[i] != 'm' && line[i] != '\0') {
                      aux = (char *)realloc(aux, (++sizep)*sizeof(char));
                      aux[sizep-1] = line[i];
                      aux[sizep] = '\0';
                      i++;
                    }
                    if (strlen(aux) == 2) {
                      if (aux[0] == '4')
                        printf("b");
                      if (strcmp(aux, "0") == 0)
                        printf("black ");
                      else if (aux[1] == '1')
                        printf("red ");
                      else if (aux[1] == '2')
                        printf("green ");
                      else if (aux[1] == '3')
                        printf("yellow ");
                      else if (aux[1] == '4')
                        printf("blue ");
                      else if (aux[1] == '5')
                        printf("magenta ");
                      else if (aux[1] == '6')
                        printf("cyan ");
                      else if (aux[1] == '7')
                        printf("white ");
                    } else {
                      if (strcmp(aux,"0") == 0)
                        printf("reset ");
                      else if (strcmp(aux,"1") == 0)
                        printf("bright ");
                      else if (strcmp(aux,"2") == 0)
                        printf("dim ");
                      else if (strcmp(aux,"4") == 0)
                        printf("underline ");
                      else if (strcmp(aux,"5") == 0)
                        printf("blink ");
                      else if (strcmp(aux,"7") == 0)
                        printf("reverse ");
                      else if (strcmp(aux,"8") == 0)
                        printf("hidden ");
                    }
                    if (line[i] == 'm') break;
                    i++;
                  }
                  break;
                } else
                    printf("%c", line[i]);
              }
              i += 2;
              break;
            case '$':
              printf("sign");
              break;
            case 'a':
              printf("bell");
              break;
            case 'D':
              printf("date "); i += 2;
              while (line[i] != '}' && line[i] != '\n') {
                printf("%c", line[i]);
                i++;
              }
              break;
            case 'h':
              printf("hostname");
              break;
            case 'H':
              printf("hostfull");
              break;
            case 'j':
              printf("jobs");
              break;
            case 'l':
              printf("terminal");
              break;
            case 'n':
              printf("newline");
              break;
            case 'r':
              printf("creturn");
              break;
            case 's':
              printf("shell");
              break;
            case 'u':
              printf("user");
              break;
            case 'v':
              printf("version");
              break;
            case 'V':
              printf("release");
              break;
            case 'w':
              printf("wdir");
              break;
            case 'W':
              printf("Wdir");
              break;
            case '!':
              printf("hisnum");
              break;
            case '#':
              printf("cmdnum");
              break;
        }
        break;
      case '$':
        if (line[i+1] == '?') {
          printf("exit"); i++;
        } else if (line[i+1] == '(') {
          i += 2;
          free(aux); aux = NULL; sizep = 0;
          while (line[i] != ')' && line[i] != '\0') {
            aux = (char *)realloc(aux, (++sizep)*sizeof(char));
            aux[sizep-1] = line[i];
            aux[sizep] = '\0';
            i++;
          }
          x = head->next;
          while (x) {
            if (strcmp(x->function, aux) == 0) {
              printf("%s", x->function);
              break;
            }
            x = x->next;
          }
        }
        break;
      case ' ':
        printf("space");
        break;
      default:
        printf("%c", line[i]);
        break;
    }
    i++;
  }

  puts("'");
  free_list(&head);
  exit(E_SUCCESS);

} // end edit_prompt function

void default_prompt(char *string) {

  DIR *dir;
  FILE *fd;
  char *aux, *line = NULL;
  struct list *head, *x;
  int size = 0;

  // First check if directory defaults exist, if not create it
  aux = malloc((strlen(getenv("HOME"))+strlen("/.epc/defaults/")+1)*sizeof(char));
  aux = strcpy(aux,getenv("HOME"));
  aux = strcat(aux, "/.epc/defaults/");
  if ((dir = opendir(aux)) == NULL)
    if (mkdir(aux, 0755))
      exit_error(aux, E_FILE);
  closedir(dir);

  if (string) {
    aux = (char *)realloc(aux, (strlen(aux)+strlen(string)+1)*sizeof(char));
    aux = strcat(aux, string);
  } else {
    aux = (char *)realloc(aux, (strlen(aux)+strlen("default")+1)*sizeof(char));
    aux = strcat(aux, "default");
  }

  fd = fopen(aux, "w+t");
  fprintf(fd, "#!/bin/bash\n"
  "export PS1=\"");
  free(aux);
  aux = malloc((strlen(getenv("PS1"))+1)*sizeof(char));
  aux = strcpy(aux, getenv("PS1"));
  // Hold on, before we need to scape the functions or they won't be actualized in the prompt
  i = 0;
  while (aux[i] !='\0') {
    if (aux[i] == '$') fprintf(fd, "\\");
    fprintf(fd, "%c", aux[i]);
    i++;
  }
  fprintf(fd, "\"\n");
  // Analize the prompt and mark the functions that need to be loaded
  i = 0; load_list(&head);
  while (aux[i] != '\0') {
    if (aux[i] == '$' && aux[i+1] == '(') {
      i += 2; free(line); line = NULL; size = 0;
      while (aux[i] != ')') {
        line = (char *)realloc(line, (++size)*sizeof(char));
        line[size-1] = aux[i];
        line[size] = '\0';
        i++;
      }
      // Compare the token with the list of functions
      x = head->next;
      while (x) {
        if (strcmp(x->function,line) == 0) {
          x->used = 1;
          break;
        }
        x = x->next;
      }
    }
    i++;
  }

  x = head->next;
  while (x) {
    if (x->used)
      if (x->defdir)
        fprintf(fd, "source "DATADIR"/functions/%s\n", x->function);
      else
        fprintf(fd, "source $HOME/.epc/functions/%s\n", x->function);
    x = x->next;
  }

  fprintf(fd, "cat >> $HOME/.epc/loadedf <<\"EOF\"\n");

  x = head->next;
  while (x) {
    if (x->used)
      fprintf(fd, "%s\n", x->function);
    x = x->next;
  }

  fprintf(fd, "EOF\n");

  fclose(fd);
  free_list(&head);

  exit(E_SUCCESS);

} // end default_prompt_function

void help(void) {

  printf(
"Usage: epc ARGUMENTS\n\n"

"EPC - Easy Prompt Changer\n"
"A simple program to make, edit or change the prompt.\n\n"

"ARGUMENTS would be a combination of OPTIONS and COMMANDS.\n"
"But ONLY ONE COMMAND is allowed and must be given.\n"
"Mandatory or optional arguments to long commands are also mandatory or optional\n"
"for any corresponding short command.\n\n"

"OPTIONS:\n"
"  -v, --verbose               Produce verbose output.\n"
"COMMANDS:\n"
"  -V, --version               Print program version.\n"
"  -h, --help                  Gives this help list you are reading.\n"
"  -n, --new='STRING'          Creates a new prompt (and change to it) using\n"
"                              the especifications given in STRING. Note that\n"
"                              STRING must be quoted. For more information\n"
"                              on how to create a prompt see:\n"
"                              \"Creation of a new prompt\" below\n\n"
"  -e, --edit=[NAME/NUMBER]    If the parameter is omited, modify the actual\n"
"                              prompt. Otherwise modify the prompt from the \n"
"                              prompts' list which matches the name or number\n"
"                              given\n"
"  -s, --save=NAME             Save the current prompt in the prompts' list\n"
"  -l, --list                  List the prompts stored\n"
"  -c, --change=NAME/NUMBER    Change to the prompt in the prompts' list\n"
"                              which matches the name or number given\n"
"  -d, --default=[TERMINAL]    Make the current prompt the default one.\n"
"                              If TERMINAL is specified then the prompt will\n"
"                              be used only in that terminal.\n\n"
"Creation of a new prompt:\n"
"To create a new prompt just give a string, preferably simple quoted\n"
"combining the following tokens. The string also may have other strings\n"
"double quoted inside, those ones will be treated literally. That is,\n"
"the words won't expand as tokens.\n\n"
"  bell        = An ASCII bell character (07)\n"
"  date format = The format is passed to strftime(3) and the result\n"
"                is inserted into the prompt string; an empty format\n"
"                results in a locale-specific time representation\n"
"  hostname    = The hostname up to the first .\n"
"  hostfull    = The hostname\n"
"  jobs        = The number of jobs currently managed by the shell\n"
"  terminal    = The basename of the shell's terminal device name\n"
"  newline     = A newline\n"
"  creturn     = A carriage return\n"
"  shell       = The name of the shell, the basename of $0\n"
"                (the portion following the final slash)\n"
"  user        = The username of the current user\n"
"  version     = The version of bash (e.g., 2.00)\n"
"  release     = The release of bash, version + patch level\n"
"                (e.g., 2.00.0)\n"
"  wdir        = The current working directory, with $HOME\n"
"                abbreviated with a tilde (uses the value of the\n"
"                PROMPT_DIRTRIM variable\n"
"  Wdir        = The basename of the current working directory,\n"
"                with $HOME abbreviated with a tilde\n"
"  hisnum      = The history number of this command\n"
"  cmdnum      = The history number of this command\n"
"  exit        = The exit code of the last executed command\n"
"  sign        = If the effective UID is 0, a #, otherwise a $\n"
"  space       = An ASCII space character (32)\n"
"  quote       = A double quote (\")\n"
"Foreground Colours:\n"
"  black, red, green, yellow, blue, magenta, cyan, white\n"
"Background Colours:\n"
"  Just prefix a \"b\" to above colours\n"
"Text attributes:\n"
"  reset, bright, dim, underline, blink, reverse, hidden\n"
);

  DIR *dir;
  struct dirent *dirdata;
  FILE *fd;
  char *aux;
  int size = 0, match;

  // Now print the help of the functios stored in the system
  if ((dir = opendir(DATADIR"/functions")) != NULL) {
    printf("Other functions:\n");
    while ((dirdata = readdir(dir)) != NULL) {
      if (dirdata->d_name[0] != '.') {
        aux = malloc((strlen(DATADIR"/functions/")+strlen(dirdata->d_name)+1)*sizeof(char));
        aux = strcpy(aux, DATADIR"/functions/");
        aux = strcat(aux, dirdata->d_name);
        fd = fopen(aux, "rt"); free(aux); aux = NULL;
        printf("  %s = ", dirdata->d_name);
        match = 0;
        while (!feof(fd)) {
          getline(&aux, &size, fd);
          aux[strlen(aux)-1] = '\0';
          if (aux[0] == '#' && aux[1] == '#' && aux[2] == '#') {
            printf("%s\n", aux+4);
            match = 1;
          }
        }
        fclose(fd);
        if (!match) printf("No help available\n");
      }
    }
  }
  closedir(dir); free(aux);
  // Do the same with the user's functions
  aux = malloc((strlen(getenv("HOME"))+strlen("/.epc/functions")+1)*sizeof(char));
  aux = strcpy(aux, getenv("HOME"));
  aux = strcat(aux, "/.epc/functions");
  if ((dir = opendir(aux)) != NULL) {
    printf("User defined functions:\n"); free(aux);
    while ((dirdata = readdir(dir)) != NULL) {
      if (dirdata->d_name[0] != '.') {
        aux = malloc((strlen(DATADIR"/functions/")+strlen(dirdata->d_name)+1)*sizeof(char));
        aux = strcpy(aux, DATADIR"/functions/");
        aux = strcat(aux, dirdata->d_name);
        fd = fopen(aux, "rt"); free(aux); aux = NULL;
        printf("  %s = ", dirdata->d_name);
        match = 0;
        while (!feof(fd)) {
          getline(&aux, &size, fd);
          aux[strlen(aux)-1] = '\0';
          if (aux[0] == '#' && aux[1] == '#' && aux[2] == '#') {
            printf("%s\n", aux+4);
            match = 1;
          }
        }
        fclose(fd);
        if (!match) printf("No help available\n");
      }
    }
  }
  closedir(dir); free(aux);
  
  exit(E_SUCCESS);

} // end help function

/** --------------------------------- Main Program ---------------------------------- **/

int main (int argc, char **argv)
{
  
  char version[] =
PACKAGE"-"VERSION;

  char usage_err[] =
"Usage: epc ARGUMENTS\n"
"Try \"epc --help\" or \"man epc\" for more information";

  static struct option long_options[] = {
    // These options set a flag
    {"verbose",  no_argument, &verbose_flag, 1},
    // These ones doesn't. They're commands
    {"version",  no_argument,        0,  'V'},
    {"help",     no_argument,        0,  'h'},
    {"new",      required_argument,  0,  'n'},
    {"edit",     optional_argument,  0,  'e'},
    {"save",     required_argument,  0,  's'},
    {"list",     no_argument,        0,  'l'},
    {"change",   required_argument,  0,  'c'},
    {"default",  optional_argument,  0,  'd'},
    {0, 0, 0, 0}    
  };

  opterr = 1; // To let getopt write error messages

  int option_index = 0; // This variable is to save the optind value

  int num = 0;  // This variable is to accept just only one command
  char cmd;     // And that is to save that command
  int arg;      // This is to save de position of the argument

  while (1) {

    i = getopt_long(argc, argv, "vVhn:e::s:lc:d::", long_options, &option_index);
    if (i == -1) break;

    switch (i) {

      case  0 : break; // If the option set a flag do nothing here
      case '?': // getopt already printed an error message, however, add a usage mesage
                // and return with a suitable error code
        puts(usage_err);
        return E_USAGE;
      case 'v':
        verbose_flag = 1;
        break;
      case 'V': case 'h': case 'n': case 'e': case 's': case 'l': case 'c': case 'd':
        num++;
        cmd = i;
        arg = optind - 1;
        break;
      default:
        break;

    } // end switch

  } // end while

  if (num == 0) {
    puts("Command needed");
    puts(usage_err);
    return E_USAGE;
  } else if (num > 1) {
    puts("Only one command at a time is supported");
    puts(usage_err);
    return E_USAGE;
  }

  switch (cmd) {

    case 'V':
      puts(version);
      break;
    case 'h':
      help();
      break;
    case 'n':
      new_prompt(argv[arg]);
      break;
    case 's':
      save_prompt(argv[arg]);
      break;
    case 'c':
      change_prompt(argv[arg]);
      break;
    case 'l':
      list_prompt();
      break;
    case 'e':
      if (argv[arg+1]) edit_prompt(argv[arg+1]);
      else edit_prompt(NULL);
      break;
    case 'd':
      if (argv[arg+1]) default_prompt(argv[arg+1]);
      else default_prompt(NULL);
      break;

  } // end switch

  return E_SUCCESS;

}
