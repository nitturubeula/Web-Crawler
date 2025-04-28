#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<curl/curl.h>
#include<regex.h>

// Node for the queue
typedef struct Node {
    char *url;
    struct Node *next;
} Node;

// Queue structure
typedef struct Queue {
    Node *front;
    Node *rear;
} Queue;

// Function to initialize the queue
Queue *initQueue() {
    Queue *queue = (Queue *)malloc(sizeof(Queue));
    if (!queue) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    queue->front = queue->rear = NULL;
    return queue;
}

// Function to add a URL to the queue
void enqueue(Queue *queue, char *url) {
    Node *temp = (Node *)malloc(sizeof(Node));
    if (!temp) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    temp->url = strdup(url);
    temp->next = NULL;
    if (queue->rear == NULL) {
        queue->front = queue->rear = temp;
    } else {
        queue->rear->next = temp;
        queue->rear = temp;
    }
}

// Function to remove a URL from the queue
char *dequeue(Queue *queue) {
    if (queue->front == NULL) return NULL;
    Node *temp = queue->front;
    char *url = strdup(temp->url);
    queue->front = queue->front->next;
    if (queue->front == NULL) queue->rear = NULL;
    free(temp->url);
    free(temp);
    return url;
}

// Function to check if the queue is empty
int isQueueEmpty(Queue *queue) {
    return queue->front == NULL;
}

// Binary Search Tree Node structure
typedef struct BSTNode {
    char *url;
    struct BSTNode *left, *right;
} BSTNode;

// Function to create a new BST node
BSTNode *createBSTNode(char *url) {
    BSTNode *newNode = (BSTNode *)malloc(sizeof(BSTNode));
    if (!newNode) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    newNode->url = strdup(url);
    newNode->left = newNode->right = NULL;
    return newNode;
}

// Function to insert a URL into the BST
BSTNode *insertBST(BSTNode *root, char *url) {
    if (root == NULL)
        return createBSTNode(url);
    int cmp = strcmp(url, root->url);
    if (cmp < 0)
        root->left = insertBST(root->left, url);
    else if (cmp > 0)
        root->right = insertBST(root->right, url);
    return root;
}

// Function to search for a URL in the BST
int searchBST(BSTNode *root, char *url) {
    if (root == NULL)
        return 0;
    int cmp = strcmp(url, root->url);
    if (cmp == 0)
        return 1;
    else if (cmp < 0)
        return searchBST(root->left, url);
    else
        return searchBST(root->right, url);
}

// Structure to hold the data downloaded from a URL
struct MemoryStruct {
    char *memory;
    size_t size;
};

// Callback function for handling downloaded data
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (ptr == NULL) {
        printf("Not enough memory\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

// Function to download a webpage's content
char *readUrl(char *url) {
    CURL *curl_handle;
    CURLcode res;

    struct MemoryStruct chunk;
    chunk.memory = malloc(1);  // Start with 1 byte of memory
    if (!chunk.memory) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }
    chunk.size = 0;

    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle = curl_easy_init();
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    res = curl_easy_perform(curl_handle);

    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        free(chunk.memory);
        chunk.memory = NULL;
    }

    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();

    return chunk.memory;
}

// Function to discover websites starting from a root URL
void discover(char *root) {
    Queue *queue = initQueue();
    BSTNode *discovered_websites = NULL;

    enqueue(queue, root);
    discovered_websites = insertBST(discovered_websites, root);

    while (!isQueueEmpty(queue)) {
        char *v = dequeue(queue);
        char *raw = readUrl(v);
        if (!raw) {
            free(v);
            continue;
        }

        const char *regexPattern = "https?://([a-zA-Z0-9_-]+\\.)+[a-zA-Z]{2,6}";
        regex_t regexCompiled;
        regmatch_t groupArray[1];

        if (regcomp(&regexCompiled, regexPattern, REG_EXTENDED)) {
            fprintf(stderr, "Could not compile regex\n");
            free(v);
            free(raw);
            continue;
        }

        char *cursor = raw;
        while (regexec(&regexCompiled, cursor, 1, groupArray, 0) == 0) {
            int offset = groupArray[0].rm_eo - groupArray[0].rm_so;
            char *match = (char *)malloc(offset + 1);
            if (!match) {
                fprintf(stderr, "Memory allocation failed\n");
                break;
            }
            strncpy(match, cursor + groupArray[0].rm_so, offset);
            match[offset] = '\0';

            if (!searchBST(discovered_websites, match)) {
                printf("Website found: %s\n", match);
                discovered_websites = insertBST(discovered_websites, match);
                enqueue(queue, match);
            }

            cursor += groupArray[0].rm_eo;
            free(match);
        }

        regfree(&regexCompiled);
        free(v);
        free(raw);
    }
}

// Main function
int main(void) {
    char *root = "https://www.google.com";
    discover(root);
    return 0;
}
