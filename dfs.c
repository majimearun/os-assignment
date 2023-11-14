#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct Graph
{
    int num_nodes;         // number of nodes in the graph
    int *visited;          // visited array
    int **matrix;          // adjacency matrix
    int *leaves;           // array to store the leaf nodes
    int n_leaves;          // number of leaf nodes
    pthread_mutex_t mutex; // mutex for graph

} Graph;

void init_graph(Graph *graph, int num_nodes)
{
    graph->num_nodes = num_nodes;

    graph->visited = (int *)malloc(sizeof(int) * num_nodes);
    graph->matrix = (int **)malloc(sizeof(int *) * num_nodes);

    for (int i = 0; i < num_nodes; i++)
    {
        graph->visited[i] = 0;

        graph->matrix[i] = (int *)malloc(sizeof(int) * num_nodes);
        for (int j = 0; j < num_nodes; j++)
        {
            graph->matrix[i][j] = 0;
        }
    }

    graph->leaves = (int *)malloc(sizeof(int) * num_nodes);
    graph->n_leaves = 0;

    pthread_mutex_init(&graph->mutex, NULL);
}

typedef struct ThreadData
{
    Graph *graph;
    int node;
} ThreadData;

void *depth_search_leaves(void *arg)
{
    ThreadData *td = (ThreadData *)arg;
    Graph *graph = td->graph;
    int node = td->node;

    pthread_t threads[graph->num_nodes];
    int n_threads = 0;

    int leaf = 1;

    for(int i = 0; i < graph->num_nodes; i++)
    {
        if(graph->matrix[node][i] == 1 && graph->visited[i] == 0)
        {
            leaf = 0;
            pthread_mutex_lock(&graph->mutex);
            graph->visited[i] = 1;
            pthread_mutex_unlock(&graph->mutex);
            ThreadData *new_td = (ThreadData *)malloc(sizeof(ThreadData));
            new_td->graph = graph;
            new_td->node = i;
            
            pthread_create(&threads[n_threads++], NULL, depth_search_leaves, (void *)new_td);
        }
        
    }

    for(int i = 0; i < n_threads; i++)
    {
        pthread_join(threads[i], NULL);
    }

    if(leaf == 1)
    {
        pthread_mutex_lock(&graph->mutex);
        graph->leaves[graph->n_leaves++] = node;
        pthread_mutex_unlock(&graph->mutex);
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

    ThreadData *td = (ThreadData *)malloc(sizeof(ThreadData));
    td->graph = &graph;
    td->node = start;

    pthread_t thread;

    pthread_create(&thread, NULL, depth_search_leaves, (void *)td);

    pthread_join(thread, NULL);

    printf("The leaf nodes are: ");

    for(int i = 0; i < graph.n_leaves; i++)
    {
        printf("%d ", graph.leaves[i] + 1);
    }

    printf("\n");

    return 0;
}