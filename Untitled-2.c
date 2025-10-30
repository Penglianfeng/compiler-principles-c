#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STATES 32
#define ALPHABET_SIZE 2 /* 0=letter, 1=digit */

typedef struct {
    int transitions[MAX_STATES][ALPHABET_SIZE];
    int epsilon[MAX_STATES][MAX_STATES];
    int state_count;
    int start;
    int accept;
} NFA;

typedef struct {
    int transitions[MAX_STATES][ALPHABET_SIZE];
    int accept[MAX_STATES];
    int state_count;
    int start;
} DFA;

static void nfa_init(NFA *nfa) {
    memset(nfa, 0, sizeof(*nfa));
    for (int i = 0; i < MAX_STATES; ++i) {
        for (int j = 0; j < ALPHABET_SIZE; ++j) {
            nfa->transitions[i][j] = -1;
        }
    }
}

static void build_identifier_nfa(NFA *nfa) {
    nfa_init(nfa);
    nfa->state_count = 2;
    nfa->start = 0;
    nfa->accept = 1;

    /* q0 --letter--> q1; q1 loops on letter/digit to model [A-Za-z][A-Za-z0-9]* */
    nfa->transitions[0][0] = 1;     /* letter */
    nfa->transitions[1][0] = 1;     /* letter loop */
    nfa->transitions[1][1] = 1;     /* digit loop */
}

static void print_nfa(const NFA *nfa, const char *title) {
    printf("%s\n", title);
    printf("States: %d, Start: q%d, Accept: q%d\n", nfa->state_count, nfa->start, nfa->accept);
    printf("Transition table (letter=0, digit=1):\n");
    printf(" State | letter | digit | epsilon\n");
    for (int i = 0; i < nfa->state_count; ++i) {
        printf("  q%-3d|  ", i);
        if (nfa->transitions[i][0] != -1) {
            printf("q%-3d", nfa->transitions[i][0]);
        } else {
            printf(" -- ");
        }
        printf("|  ");
        if (nfa->transitions[i][1] != -1) {
            printf("q%-3d", nfa->transitions[i][1]);
        } else {
            printf(" -- ");
        }
        printf("| ");
        int eps_printed = 0;
        for (int j = 0; j < nfa->state_count; ++j) {
            if (nfa->epsilon[i][j]) {
                printf("q%d ", j);
                eps_printed = 1;
            }
        }
        if (!eps_printed) {
            printf("--");
        }
        printf("\n");
    }
    printf("\n");
}

static int epsilon_closure(const NFA *nfa, int state, int *closure) {
    int stack[MAX_STATES];
    int top = -1;
    int visited[MAX_STATES] = {0};
    stack[++top] = state;
    visited[state] = 1;
    int count = 0;

    while (top >= 0) {
        int s = stack[top--];
        closure[count++] = s;
        for (int t = 0; t < nfa->state_count; ++t) {
            if (nfa->epsilon[s][t] && !visited[t]) {
                visited[t] = 1;
                stack[++top] = t;
            }
        }
    }
    return count;
}

static int move(const NFA *nfa, int state, int symbol, int *dest) {
    int next = nfa->transitions[state][symbol];
    if (next == -1) return 0;
    dest[0] = next;
    return 1;
}

static int subset_construction(const NFA *nfa, DFA *dfa) {
    memset(dfa, 0, sizeof(*dfa));
    for (int i = 0; i < MAX_STATES; ++i) {
        for (int j = 0; j < ALPHABET_SIZE; ++j) {
            dfa->transitions[i][j] = -1;
        }
    }

    int state_sets[MAX_STATES];
    int queue[MAX_STATES];
    int head = 0, tail = 0;

    int closure[MAX_STATES];
    int count = epsilon_closure(nfa, nfa->start, closure);
    int initial = 0;
    for (int i = 0; i < count; ++i) {
        initial |= (1 << closure[i]);
    }

    state_sets[0] = initial;
    queue[tail++] = 0;
    dfa->start = 0;
    dfa->state_count = 1;

    if (initial & (1 << nfa->accept)) {
        dfa->accept[0] = 1;
    }

    while (head < tail) {
        int current_index = queue[head++];
        int current_set = state_sets[current_index];

        for (int symbol = 0; symbol < ALPHABET_SIZE; ++symbol) {
            int dest_mask = 0;
            for (int state = 0; state < nfa->state_count; ++state) {
                if (!(current_set & (1 << state))) {
                    continue;
                }
                int dest_states[MAX_STATES];
                int dest_count = move(nfa, state, symbol, dest_states);
                for (int i = 0; i < dest_count; ++i) {
                    int closure_states[MAX_STATES];
                    int closure_count = epsilon_closure(nfa, dest_states[i], closure_states);
                    for (int j = 0; j < closure_count; ++j) {
                        dest_mask |= (1 << closure_states[j]);
                    }
                }
            }
            if (dest_mask == 0) {
                continue;
            }

            int target_index = -1;
            for (int i = 0; i < dfa->state_count; ++i) {
                if (state_sets[i] == dest_mask) {
                    target_index = i;
                    break;
                }
            }

            if (target_index == -1) {
                target_index = dfa->state_count;
                state_sets[dfa->state_count] = dest_mask;
                queue[tail++] = target_index;
                if (dest_mask & (1 << nfa->accept)) {
                    dfa->accept[dfa->state_count] = 1;
                }
                dfa->state_count++;
            }

            dfa->transitions[current_index][symbol] = target_index;
        }
    }

    return 0;
}

static void ensure_total_dfa(DFA *dfa) {
    int trap_index = -1;
    for (int state = 0; state < dfa->state_count; ++state) {
        for (int symbol = 0; symbol < ALPHABET_SIZE; ++symbol) {
            if (dfa->transitions[state][symbol] != -1) {
                continue;
            }
            if (trap_index == -1) {
                trap_index = dfa->state_count;
                if (trap_index >= MAX_STATES) {
                    fprintf(stderr, "DFA requires a trap state but MAX_STATES is too small.\n");
                    return;
                }
                dfa->state_count++;
                for (int sym = 0; sym < ALPHABET_SIZE; ++sym) {
                    dfa->transitions[trap_index][sym] = trap_index;
                }
                dfa->accept[trap_index] = 0;
            }
            dfa->transitions[state][symbol] = trap_index;
        }
    }
}

static void hopcroft_minimize(const DFA *dfa, DFA *minimized) {
    int partition[MAX_STATES];
    int block_count = 2;
    int blocks[MAX_STATES][MAX_STATES];
    int block_sizes[MAX_STATES];

    int accept_block = 0;
    int non_accept_block = 1;

    block_sizes[accept_block] = 0;
    block_sizes[non_accept_block] = 0;

    for (int i = 0; i < dfa->state_count; ++i) {
        if (dfa->accept[i]) {
            blocks[accept_block][block_sizes[accept_block]++] = i;
            partition[i] = accept_block;
        } else {
            blocks[non_accept_block][block_sizes[non_accept_block]++] = i;
            partition[i] = non_accept_block;
        }
    }
    if (block_sizes[accept_block] == 0) {
        block_sizes[accept_block] = block_sizes[block_count - 1];
        for (int i = 0; i < block_sizes[accept_block]; ++i) {
            blocks[accept_block][i] = blocks[block_count - 1][i];
        }
        block_count--;
    }
    if (block_sizes[non_accept_block] == 0) {
        block_sizes[non_accept_block] = block_sizes[block_count - 1];
        for (int i = 0; i < block_sizes[non_accept_block]; ++i) {
            blocks[non_accept_block][i] = blocks[block_count - 1][i];
        }
        block_count--;
    }

    int changed = 1;
    while (changed) {
        changed = 0;
        for (int b = 0; b < block_count; ++b) {
            if (block_sizes[b] <= 1) continue;
            int representative = blocks[b][0];
            int split[MAX_STATES];
            int split_size = 0;
            for (int i = 1; i < block_sizes[b]; ++i) {
                int state = blocks[b][i];
                int distinguishable = 0;
                for (int symbol = 0; symbol < ALPHABET_SIZE; ++symbol) {
                    int t1 = dfa->transitions[representative][symbol];
                    int t2 = dfa->transitions[state][symbol];
                    if (partition[t1] != partition[t2]) {
                        distinguishable = 1;
                        break;
                    }
                }
                if (distinguishable) {
                    split[split_size++] = state;
                }
            }
            if (split_size > 0) {
                changed = 1;
                block_sizes[b] -= split_size;
                int new_block = block_count++;
                block_sizes[new_block] = split_size;
                for (int i = 0; i < split_size; ++i) {
                    blocks[new_block][i] = split[i];
                    for (int j = 0; j < block_sizes[b] + split_size; ++j) {
                        if (blocks[b][j] == split[i]) {
                            blocks[b][j] = blocks[b][block_sizes[b] + split_size - 1];
                            break;
                        }
                    }
                    partition[split[i]] = new_block;
                }
            }
        }
    }

    memset(minimized, 0, sizeof(*minimized));
    for (int i = 0; i < block_count; ++i) {
        minimized->state_count++;
        for (int j = 0; j < block_sizes[i]; ++j) {
            if (blocks[i][j] == dfa->start) {
                minimized->start = i;
            }
            if (dfa->accept[blocks[i][j]]) {
                minimized->accept[i] = 1;
            }
        }
    }
    for (int i = 0; i < block_count; ++i) {
        int representative = blocks[i][0];
        for (int symbol = 0; symbol < ALPHABET_SIZE; ++symbol) {
            int target = dfa->transitions[representative][symbol];
            minimized->transitions[i][symbol] = partition[target];
        }
    }
}

static void print_dfa(const DFA *dfa, const char *title) {
    printf("%s\n", title);
    printf("States: %d, Start: q%d\n", dfa->state_count, dfa->start);
    printf("Accepting:");
    for (int i = 0; i < dfa->state_count; ++i) {
        if (dfa->accept[i]) {
            printf(" q%d", i);
        }
    }
    printf("\nTransition table:\n");
    printf(" State | letter | digit\n");
    for (int i = 0; i < dfa->state_count; ++i) {
        printf("  q%-3d|  ", i);
        if (dfa->transitions[i][0] != -1) {
            printf("q%-3d", dfa->transitions[i][0]);
        } else {
            printf(" -- ");
        }
        printf("|  ");
        if (dfa->transitions[i][1] != -1) {
            printf("q%-3d", dfa->transitions[i][1]);
        } else {
            printf(" -- ");
        }
        printf("\n");
    }
    printf("\n");
}

static void export_dfa_matrix(const DFA *dfa) {
    printf("Matrix form (rows=states, cols=letter,digit):\n");
    for (int i = 0; i < dfa->state_count; ++i) {
        printf("q%-3d -> [%2d %2d]%s\n", i,
               dfa->transitions[i][0],
               dfa->transitions[i][1],
               dfa->accept[i] ? " *" : "");
    }
    printf("\n");
}

int main(void) {
    NFA nfa;
    DFA dfa;
    DFA minimized;

    build_identifier_nfa(&nfa);
    print_nfa(&nfa, "Identifier NFA");

    subset_construction(&nfa, &dfa);
    ensure_total_dfa(&dfa);
    print_dfa(&dfa, "DFA before minimization");
    export_dfa_matrix(&dfa);

    hopcroft_minimize(&dfa, &minimized);
    print_dfa(&minimized, "Minimal DFA");
    export_dfa_matrix(&minimized);

    printf("Start state (minimal DFA): q%d\n", minimized.start);
    printf("Accepting states (minimal DFA):");
    for (int i = 0; i < minimized.state_count; ++i) {
        if (minimized.accept[i]) {
            printf(" q%d", i);
        }
    }
    printf("\n");

    return EXIT_SUCCESS;
}
