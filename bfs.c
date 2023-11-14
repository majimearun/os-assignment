#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct Graph
{
    int num_nodes;         // number of nodes in the graph
    int *visited;          // visited array
    int *current_level;    // nodes visited in current layer
    int *next_level;       // nodes that need to be visited in next layer
    int **matrix;          // adjacency matrix
    int nextc;             // index of the current_level array
    int nextn;             // index of the next_level array
    int left;              // number of nodes left to be visited
    pthread_mutex_t mutex; // mutex for graph
    int *traversal;        // array to store the travesal
    int t_index;           // index of traversal array

} Graph;

void init_graph(Graph *graph, int num_nodes)
{
    graph->num_nodes = num_nodes;

    graph->visited = (int *)malloc(sizeof(int) * num_nodes);
    graph->current_level = (int *)malloc(sizeof(int) * num_nodes);
    graph->next_level = (int *)malloc(sizeof(int) * num_nodes);
    graph->matrix = (int **)malloc(sizeof(int *) * num_nodes);
    graph->traversal = (int *)malloc(sizeof(int) * num_nodes);
    graph->t_index = 0;

    for (int i = 0; i < num_nodes; i++)
    {
        graph->visited[i] = 0;
        graph->current_level[i] = 0;
        graph->next_level[i] = 0;
        graph->matrix[i] = (int *)malloc(sizeof(int) * num_nodes);
        for (int j = 0; j < num_nodes; j++)
        {
            graph->matrix[i][j] = 0;
        }
    }

    graph->nextc = 0;
    graph->nextn = 0;
    graph->left = num_nodes;

    pthread_mutex_init(&graph->mutex, NULL);
}

typedef struct ThreadData
{
    Graph *graph;
    int node;
} ThreadData;

void *add_to_next_level(void *arg)
{
    // printf("In thread for %d\n", ((ThreadData *)arg)->node + 1);
    ThreadData *td = (ThreadData *)arg;
    Graph *graph = td->graph;
    int node = td->node;

    for (int i = 0; i < graph->num_nodes; i++)
    {
        if (graph->matrix[node][i] == 1 && graph->visited[i] == 0)
        {
            pthread_mutex_lock(&graph->mutex);
            graph->traversal[graph->t_index++] = i;
            graph->next_level[graph->nextn++] = i;
            graph->visited[i] = 1;
            graph->left--;
            pthread_mutex_unlock(&graph->mutex);
        }
    }

    pthread_exit(NULL);
}

int main()
{

    int num;
    char name[100];
    printf("Enter the name of the file: ");
    scanf("%s", name);
    FILE *fp = fopen(name, "r");
    fscanf(fp, "%d", &num);
    int matrix[num][num];

    for (int i = 0; i < num; i++)
    {
        for (int j = 0; j < num; j++)
            fscanf(fp, "%d", &matrix[i][j]);
    }

    Graph graph;
    init_graph(&graph, num);
    for (int i = 0; i < num; i++)
    {
        for (int j = 0; j < num; j++)
            graph.matrix[i][j] = matrix[i][j];
    }

    int start;
    printf("Enter the starting node: ");
    scanf("%d", &start);
    start--;
    graph.visited[start] = 1;
    graph.current_level[graph.nextc++] = start;

    pthread_t threads[num];
    int n_threads = 0;

    graph.left--;
    while (graph.left > 0)
    {
        // printf("-----------------------------\n");
        // printf("Left: %d\n", graph.left);
        // printf("In current level: %d\n", graph.nextc);
        // if (graph.nextc == 0)
        // {
        //     printf("No more nodes to visit: error in code or isolated nodes present\n");
        //     break;
        // }

        for (int i = 0; i < graph.nextc; i++)
        {
            ThreadData *td = (ThreadData *)malloc(sizeof(ThreadData));
            td->graph = &graph;
            td->node = graph.current_level[i];
            // printf("Creating thread for %d\n", td->node + 1);
            pthread_create(&threads[n_threads++], NULL, add_to_next_level, (void *)td);
        }

        // printf("Waiting for threads to finish...\n");
        for (int i = 0; i < n_threads; i++)
        {
            pthread_join(threads[i], NULL);
        }

        free(graph.current_level);
        graph.current_level = graph.next_level;
        graph.next_level = (int *)malloc(sizeof(int) * num);
        graph.nextc = graph.nextn;
        graph.nextn = 0;

        n_threads = 0;
    }

    printf("%d ", start + 1);
    for(int i = 0; i < graph.t_index; i++)
    {
        printf("%d ", graph.traversal[i] + 1);
    }
    printf("\n");
    return 0;
}