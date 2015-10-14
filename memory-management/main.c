#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>


#define EMPTY_SPACE 255
#define PROCESS_NAME_MAX_LENGHT 16
#define MAX_PROCESS_SIZE 32
#define MAX_ACCESS_REQUEST 16
struct access_request {
  float t;
  int p;
};

typedef struct process_definition {
  float t0;
  float tf;
  char *name;
  int b;
  struct access_request access_requests[MAX_ACCESS_REQUEST];
  int access_requests_counter;
  char pid;
} *ProcessDefinition;

typedef struct experiment {
  ProcessDefinition *trials;
  int trials_counter;
} * Experiment;

struct memory_map {
  int begin;
  int end;
};

/******************************************************************************/
struct memory_map MEMORY_MAPPER[257];

/******************************************************************************/
FILE * generate_memory_file(int size);
FILE * generate_virtual_memory_file(int size);
Experiment generate_experiment(FILE *);
void * perform( void *);
void memory_request(ProcessDefinition, struct access_request);

/******************************************************************************/
int main( int argc, char *argv[]) {
  char *tracefile_name;
  FILE *tracefile, *memory_file, *virtual_memory_file;
  int memory_size, virtual_memory_size;
  Experiment experiment;
  int i, j;
  struct access_request access_request;
  size_t len = 0;
  ssize_t read;
  pthread_t *threads;
  char *line = malloc(128);
  char c;

  tracefile_name = argv[1];
  tracefile = fopen( tracefile_name, "r");
  if( tracefile) {
    read = getline(&line, &len, tracefile);
    sscanf( line, "%d %d", &memory_size, &virtual_memory_size);
  } else {
    return 1;
  }

  memory_file = generate_memory_file(memory_size);
  fclose(memory_file);
  virtual_memory_file = generate_virtual_memory_file(virtual_memory_size);
  fclose(virtual_memory_file);

  MEMORY_MAPPER[0].begin = -1;
  MEMORY_MAPPER[0].end = -1;

  experiment = generate_experiment(tracefile);

  threads = malloc(sizeof(* threads) * experiment->trials_counter);
  for(j = 0; j < experiment->trials_counter; j++) {
    pthread_create(&threads[j], NULL, perform, experiment->trials[j]);
  }

  for(j = 0; j < experiment->trials_counter; j++) {
    pthread_join(threads[j], NULL);
  }

  memory_file = fopen("/tmp/ep2.mem", "rb");
  while(fscanf(memory_file, "%c", &c) != EOF) {
    printf("%hhd", c);
  }
  fclose(memory_file);

  return 0;
}

/******************************************************************************/
FILE * generate_memory_file(int size) {
  FILE *file = fopen( "/tmp/ep2.mem", "w");
  short byte = EMPTY_SPACE;
  int i;
  for( i = 0; i < size; i++) fwrite( &byte, 1, 1, file);
  return file;
}

FILE * generate_virtual_memory_file(int size) {
  FILE *file = fopen("/tmp/ep2.vir", "wb");
  char byte = EMPTY_SPACE;

  int i;
  for( i = 0; i < size; i++) fwrite( &byte, 1, 1, file);
  return file;
}

Experiment generate_experiment(FILE *tracefile) {
  Experiment experiment = malloc(sizeof(*experiment));
  ProcessDefinition process_definition;
  int access_requests_counter, trials_counter, pi;
  float ti;
  int end_of_file = 0;
  char *line = malloc(128);
  size_t len = 0;
  ssize_t read;
  char *token;
  char  *str;
  float f;
  int i;
  char pid = 1;

  experiment->trials = malloc(sizeof(ProcessDefinition)* MAX_PROCESS_SIZE);
  trials_counter = 0;
  while( (read = getline(&line, &len, tracefile)) > 0) {
    process_definition = malloc( sizeof( *process_definition));

    token = strsep(&line, " ");
    sscanf(token, "%f", &f);
    process_definition->t0 = f;

    process_definition->name = strsep(&line, " ");

    token = strsep(&line, " ");
    sscanf(token, "%f", &f);
    process_definition->tf = f;

    token = strsep(&line, " ");
    sscanf(token, "%d", &i);
    process_definition->b = i;

    access_requests_counter = 0;
    while(line != NULL) {
      token = strsep(&line, " ");
      sscanf(token, "%f", &ti);

      token = strsep(&line, " ");
      sscanf(token, "%d", &pi);

      process_definition->access_requests[access_requests_counter].t = ti;
      process_definition->access_requests[access_requests_counter].p = pi;
      access_requests_counter++;
    }

    process_definition->access_requests_counter = access_requests_counter;
    MEMORY_MAPPER[pid].begin = MEMORY_MAPPER[pid - 1].end;
    MEMORY_MAPPER[pid].end = MEMORY_MAPPER[pid].begin + process_definition->b;
    process_definition->pid = pid++;
    experiment->trials[trials_counter] = process_definition;
    trials_counter++;
  }

  experiment->trials_counter = trials_counter;
  return experiment;
}

void * perform(void *argument)
{
  ProcessDefinition pd = argument;
  struct timeval start, now;
  long elapsed_time = 0;
  int access_request_index = 0;

  gettimeofday(&start, NULL);
  usleep(pd->t0 * 1000);
  printf("starting %s\n", pd->name);

  while (elapsed_time < pd->tf)
  {
    gettimeofday(&now, NULL);
    elapsed_time = (now.tv_sec - start.tv_sec);

    if(pd->access_requests[access_request_index].t <= elapsed_time) {
      memory_request(pd, pd->access_requests[access_request_index]);
      access_request_index++;
    }
  }

  printf("%s time: %ld\n", pd->name, elapsed_time);
  return NULL;
}

void memory_request(ProcessDefinition pd, struct access_request ar) {
  FILE * memory_file = fopen("/tmp/ep2.mem", "r+b");
  fseek(memory_file, MEMORY_MAPPER[pd->pid].begin + ar.p, SEEK_SET);
  fwrite(&pd->pid, 1, 1, memory_file);
  fclose(memory_file);

  // printf("%s requested the position %d at time %f\n", pd->name, ar.p, ar.t);
  // printf("writing %hhd at position %d\n", pd->pid, MEMORY_MAPPER[pd->pid].begin + ar.p);
}