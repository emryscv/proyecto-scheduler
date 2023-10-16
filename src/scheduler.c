#include "scheduler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include "simulation.h"

// La función que define un scheduler está compuesta por los siguientes
// parámetros:
//
//  - procs_info: Array que contiene la información de cada proceso activo
//  - procs_count: Cantidad de procesos activos
//  - curr_time: Tiempo actual de la simulación
//  - curr_pid: PID del proceso que se está ejecutando en el CPU
//
// Esta función se ejecuta en cada timer-interrupt donde existan procesos
// activos (se asegura que `procs_count > 0`) y determina el PID del proceso a
// ejecutar. El valor de retorno es un entero que indica el PID de dicho
// proceso. Pueden ocurrir tres casos:
//
//  - La función devuelve -1: No se ejecuta ningún proceso.
//  - La función devuelve un PID igual al curr_pid: Se mantiene en ejecución el
//  proceso actual.
//  - La función devuelve un PID diferente al curr_pid: Simula un cambio de
//  contexto y se ejecuta el proceso indicado.
//

int currentIndexRR = 0;
int amountTimerInterrupts = 0;

int index_last_pid = 0; // para saber la posicion en procs_info del curr_pid

int last_pid = -1; // para saber cual es el pid si el programa termino

int current_index_RR_q[3];
; // para saber que programa le toca en el round robin de mlfq

int last_of_q[3]; // punteros para emular una lista

int q[MAX_PROCESS_COUNT][3]; // listas de prioridades mayor 0 menor 2

int exececuted_timer_interrupts[MAX_PROCESS_COUNT];

proc_info_t *pid_to_proc_info[MAX_PROCESS_COUNT];

int mlfq_first_time = 1;
int mlfq_firt_process = 1;
int process_priority[MAX_PROCESS_COUNT];

int fifo_scheduler(proc_info_t *procs_info, int procs_count, int curr_time,
                   int curr_pid)
{
  // Se devuelve el PID del primer proceso de todos los disponibles (los
  // procesos están ordenados por orden de llegada).
  return procs_info[0].pid;
}

// testear

int sjf_scheduler(proc_info_t *procs_info, int procs_count, int curr_time,
                  int curr_pid)
{
  int minTime = INT_MAX;
  int index = 0;

  for (int i = 0; i < procs_count; i++)
  { // buscar el proceso con menor tiempo de ejecucion total
    int _processTotalTime = process_total_time(procs_info[i].pid);
    if (_processTotalTime < minTime)
    {
      index = i;
      minTime = _processTotalTime;
    }
  }

  return procs_info[index].pid;
}

// testear
int stcf_scheduler(proc_info_t *procs_info, int procs_count, int curr_time,
                   int curr_pid)
{
  int minTime = INT_MAX;
  int index = 0;

  for (int i = 0; i < procs_count; i++)
  { // buscar el proceso con menor tiempo restante (total - transcurrido)
    int _processTotalTime = process_total_time(procs_info[i].pid);
    if (_processTotalTime - procs_info[i].executed_time < minTime)
    {
      index = i;
      minTime = _processTotalTime - procs_info[i].executed_time;
    }
  }

  return procs_info[index].pid;
}

// testear
// como averiguo cuanto le falta por terminar al proceso curr_pid

int round_robin_scheduler(proc_info_t *procs_info, int procs_count, int curr_time,
                          int curr_pid, int time_slice)
{ // time_slice sería cuantos timer-interupt son necesarios para cambiar de proceso
  time_slice = 5;

  if (amountTimerInterrupts < time_slice - 1 && curr_pid != -1 && !procs_info[index_last_pid].on_io)
  { // si aun no se cumple el time slice seguimos con el mismo proceso
    amountTimerInterrupts++;
    return curr_pid;
  }
  else
  { // toca cambiar de proceso
    int _pid;

    // aqui se contempla la posibilidad de que el proceso termino en el time slice pasado entonces no es necessario
    // actualizar el index solo si el proceso se mantiene en su posicion en el array
    if (procs_info[currentIndexRR].pid == curr_pid && curr_pid != -1)
    {
      currentIndexRR++;
    }

    while (currentIndexRR < procs_count && procs_info[currentIndexRR].on_io)
    {
      currentIndexRR++;
    }

    if (currentIndexRR >= procs_count)
    { // cuando ejecutamos el ultimo en la lista le toca al del incio otra vez
      currentIndexRR = 0;
    }

    _pid = procs_info[currentIndexRR].pid;
    amountTimerInterrupts = 0;

    return _pid;
  }
}

void add_to_queue(int priority, int pid, int flag)
{
  q[last_of_q[priority]++][priority] = pid;
  process_priority[pid] = priority;
  if(flag){
    exececuted_timer_interrupts[pid] = 0;
  }
}

void remove_from_queue(int priority, int pid)
{
  int i = 0;
  while (q[i][priority] != pid)
  {
    i++;
  }

  while (i < last_of_q[priority] - 1)
  {
    q[i][priority] = q[i + 1][priority];
    i++;
  }
  last_of_q[priority]--;
}

int mlfq_scheduler(proc_info_t *procs_info, int procs_count, int curr_time,
                   int curr_pid, int time_slice, int priority_boost)
{
  time_slice = 5;
  priority_boost = 100;

  //incializar las estructuras de datos necesarias
  if (mlfq_first_time)
  {
    for (int i = 0; i < MAX_PROCESS_COUNT; i++)
    {
      process_priority[i] = -1;
    }
    for (int i = 0; i < 3; i++)
    {
      current_index_RR_q[i] = 0;
      last_of_q[i] = 0;
    }
    mlfq_first_time = 0;
  }

  // si el programa termino quitarlo de las listas
  if (!mlfq_firt_process && curr_pid == -1)
  {
    remove_from_queue(process_priority[last_pid], last_pid);
  }
  else
  { // cambiar la prioridad regla 4
    if (exececuted_timer_interrupts[curr_pid] >= time_slice-1 && !pid_to_proc_info[curr_pid]->on_io)
    {
      if (process_priority[curr_pid] < 2)
      {

        int _priority = process_priority[curr_pid]; 
        add_to_queue(process_priority[curr_pid] + 1, curr_pid, 0);
        remove_from_queue(_priority, curr_pid);
      }
      //curr_pid = -1;
    }
  }

  // agregar proceso nuevo a q0 regla 3
  for (int i = 0; i < procs_count; i++)
  {
    if (process_priority[procs_info[i].pid] == -1)
    {
      process_priority[procs_info[i].pid] = 0;
      add_to_queue(0, procs_info[i].pid, 1);
      pid_to_proc_info[procs_info[i].pid] = &procs_info[i];
      mlfq_firt_process = 0; // marcar que ya se hizo un proceso
    }
  }

  // priority_boost regla 5
  if (curr_time % (priority_boost * 10) == 0)
  {
    for (int i = 0; i < last_of_q[2]; i++)
    {
      add_to_queue(0, q[i][2], 1);
    }
    for (int i = 0; i < last_of_q[1]; i++)
    {
      add_to_queue(0, q[i][1], 1);
    }
    last_of_q[2] = 0;
    last_of_q[1] = 0;
  }

  // econtrar elementos de mayor prioridad regla 1
  int priority = 0;
  while (last_of_q[priority] == 0 && priority <= 2)
  {
    
    
    priority++;
  }
  
   // round robin regla 2
  if (exececuted_timer_interrupts[curr_pid] < time_slice - 1 && curr_pid != -1 && !procs_info[index_last_pid].on_io)
  { // si aun no se cumple el time slice y no esta en io seguimos con el mismo proceso
     exececuted_timer_interrupts[curr_pid]++;
     return curr_pid;
  }
  else
  { // toca cambiar de proceso
    if (exececuted_timer_interrupts[curr_pid] >= time_slice - 1 && !pid_to_proc_info[curr_pid]->on_io){
      exececuted_timer_interrupts[curr_pid] = 0;
    }
    if (q[current_index_RR_q[priority]][priority] == curr_pid)
    {
      current_index_RR_q[priority]++;
    }

    while (priority <= 2)
    {
      while (current_index_RR_q[priority] < last_of_q[priority] && pid_to_proc_info[q[current_index_RR_q[priority]][priority]]->on_io)
      {
        printf("%d %d\n", q[current_index_RR_q[priority]][priority], pid_to_proc_info[q[current_index_RR_q[priority]][priority]]->on_io);
        current_index_RR_q[priority]++;
      }

      if (current_index_RR_q[priority] >= last_of_q[priority])
      { // cuando ejecutamos el ultimo en la lista le toca al del incio otra vez
        current_index_RR_q[priority] = 0;
      }

      while (current_index_RR_q[priority] < last_of_q[priority] && pid_to_proc_info[q[current_index_RR_q[priority]][priority]]->on_io)
      {
        current_index_RR_q[priority]++;
      }

      // todos los elementos estan io
      if (current_index_RR_q[priority] >= last_of_q[priority])
      { // cuando ejecutamos el ultimo en la lista le toca al del incio otra vez
        priority++;
        continue;
      }

      last_pid = q[current_index_RR_q[priority]][priority];
      return last_pid;
    }
    if(last_of_q[0] > 0){
      return q[0][0];
    }
    if(last_of_q[1] > 0){
      return q[0][1];
    }
    if(last_of_q[2] > 0){
      return q[0][2];
    }
  }
}

int my_own_scheduler(proc_info_t *procs_info, int procs_count, int curr_time,
                     int curr_pid)
{
  // Implementa tu scheduler aqui ... (el nombre de la función lo puedes
  // cambiar)

  // Información que puedes obtener de un proceso
  int pid = procs_info[0].pid;                 // PID del proceso
  int on_io = procs_info[0].on_io;             // Indica si el proceso se encuentra
                                               // realizando una opreación IO
  int exec_time = procs_info[0].executed_time; // Tiempo que el proceso se ha
                                               // ejecutado (en CPU o en I/O)

  // También puedes usar funciones definidas en `simulation.h` para extraer
  // información extra:
  int duration = process_total_time(pid);

  return -1;
}

// Esta función devuelve la función que se ejecutará en cada timer-interrupt
// según el nombre del scheduler.
schedule_action_t get_scheduler(const char *name)
{
  // Si necesitas inicializar alguna estructura antes de comenzar la simulación
  // puedes hacerlo aquí.

  if (strcmp(name, "fifo") == 0)
    return *fifo_scheduler;
  if (strcmp(name, "sjf") == 0)
    return *sjf_scheduler;
  if (strcmp(name, "stcf") == 0)
    return *stcf_scheduler;
  if (strcmp(name, "rr") == 0)
    return *round_robin_scheduler;
  if (strcmp(name, "mlfq") == 0)
    return *mlfq_scheduler;
  // Añade aquí los schedulers que implementes. Por ejemplo:
  //
  // if (strcmp(name, "sjf") == 0) return *sjf_scheduler;
  //

  fprintf(stderr, "Invalid scheduler name: '%s'\n", name);
  exit(1);
}
