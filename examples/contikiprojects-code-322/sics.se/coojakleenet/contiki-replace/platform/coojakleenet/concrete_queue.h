#ifndef __CONCRETE_QUEUE_H__
#define __CONCRETE_QUEUE_H__

void concrete_queue_getnext(void*, int, char*);
int concrete_queue_getsymbol(int, char*);
void concrete_queue_init(int);

extern int concrete_queue_error;

#if COOJAKLEENET

/* platform/coojakleenet */
#define KLEENET_ASSERT(expr) klee_assert(expr)
#define KLEENET_SYMBOL(dest, destid, symbol) do { \
                                        kleenet_get_global_symbol(dest, symbol, sizeof(int), destid); \
                                       } while (0)
#define KLEENET_SYMBOLIC_MEMORY(address, size, name) klee_make_symbolic(address, size, name)
#define KLEENET_EXIT(message) kleenet_early_exit(#message)
#define KLEENET_SILENT_EXIT(void) klee_silent_exit(0)

#else /* COOJAKLEENET */

/* platform/cooja */
#define KLEENET_SILENT_EXIT(void) do { \
                                concrete_queue_error = 1; \
                                printf("-- Simulation stopped --\n"); \
                              } while (0)
#define KLEENET_EXIT(message) do { \
                                concrete_queue_error = 1; \
                                printf("-- Simulation stopped: %s --\n", #message); \
                              } while (0)
#define KLEENET_ASSERT(expr) do { \
                               if (!(expr)) { \
                                 concrete_queue_error = 1; \
                                 printf("-- ASSERTION %s FAILED --\n", #expr); \
                               } \
                             } while (0)
#define KLEENET_SYMBOL(dest, destid, symbol) concrete_queue_getsymbol(destid, symbol)
#define KLEENET_SYMBOLIC_MEMORY(address, size, name) do { \
                               concrete_queue_getnext(address, size, name); \
                             } while (0)

#endif /* COOJAKLEENET */

#endif /* __CONCRETE_QUEUE_H__ */
