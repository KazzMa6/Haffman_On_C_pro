// huffman_unicode_windows.c

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <locale.h>
#include <string.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#define MAX_TREE_HT 256
#define MAX_SYMBOLS 65536 // все возможные wchar_t символы

typedef struct HuffmanNode {
    wchar_t symbol;
    int frequency;
    struct HuffmanNode* left;
    struct HuffmanNode* right;
} HuffmanNode;

typedef struct MinHeap {
    int size;
    int capacity;
    HuffmanNode** array;
} MinHeap;

// Создание узла дерева
HuffmanNode* createNode(wchar_t symbol, int frequency) {
    HuffmanNode* node = (HuffmanNode*)malloc(sizeof(HuffmanNode));
    node->symbol = symbol;
    node->frequency = frequency;
    node->left = NULL;
    node->right = NULL;
    return node;
}

// Создание мин-кучи
MinHeap* createMinHeap(int capacity) {
    MinHeap* heap = (MinHeap*)malloc(sizeof(MinHeap));
    heap->size = 0;
    heap->capacity = capacity;
    heap->array = (HuffmanNode**)malloc(sizeof(HuffmanNode*) * capacity);
    return heap;
}

// Обмен двух указателей на узлы
void swapNodes(HuffmanNode** a, HuffmanNode** b) {
    HuffmanNode* temp = *a;
    *a = *b;
    *b = temp;
}

// Восстановление свойств мин-кучи
void minHeapify(MinHeap* heap, int i) {
    int smallest = i;
    int l = 2 * i + 1;
    int r = 2 * i + 2;

    if (l < heap->size && heap->array[l]->frequency < heap->array[smallest]->frequency)
        smallest = l;
    if (r < heap->size && heap->array[r]->frequency < heap->array[smallest]->frequency)
        smallest = r;

    if (smallest != i) {
        swapNodes(&heap->array[i], &heap->array[smallest]);
        minHeapify(heap, smallest);
    }
}

int isSizeOne(MinHeap* heap) {
    return (heap->size == 1);
}

HuffmanNode* extractMin(MinHeap* heap) {
    HuffmanNode* temp = heap->array[0];
    heap->array[0] = heap->array[--heap->size];
    minHeapify(heap, 0);
    return temp;
}

void insertMinHeap(MinHeap* heap, HuffmanNode* node) {
    int i = heap->size++;
    while (i && node->frequency < heap->array[(i - 1) / 2]->frequency) {
        heap->array[i] = heap->array[(i - 1) / 2];
        i = (i - 1) / 2;
    }
    heap->array[i] = node;
}

void buildMinHeap(MinHeap* heap) {
    for (int i = (heap->size - 1) / 2; i >= 0; i--)
        minHeapify(heap, i);
}

// Проверка на лист дерева
int isLeaf(HuffmanNode* node) {
    return !(node->left) && !(node->right);
}

// Создание мин-кучи из таблицы частот
MinHeap* buildAndCreateMinHeap(int freq[]) {
    MinHeap* heap = createMinHeap(MAX_SYMBOLS);
    for (int i = 0; i < MAX_SYMBOLS; i++) {
        if (freq[i] > 0)
            heap->array[heap->size++] = createNode((wchar_t)i, freq[i]);
    }
    buildMinHeap(heap);
    return heap;
}

// Построение дерева Хаффмана
HuffmanNode* buildHuffmanTree(int freq[]) {
    HuffmanNode *left, *right, *top;
    MinHeap* heap = buildAndCreateMinHeap(freq);

    while (!isSizeOne(heap)) {
        left = extractMin(heap);
        right = extractMin(heap);
        top = createNode(L'\0', left->frequency + right->frequency);
        top->left = left;
        top->right = right;
        insertMinHeap(heap, top);
    }

    return extractMin(heap);
}

// Массив для хранения кодов
wchar_t* huffmanCodes[MAX_SYMBOLS] = {0};

// Построение кодов символов
void buildCodes(HuffmanNode* root, wchar_t* code, int depth) {
    if (!root) return;

    // Если это лист — сохраняем код
    if (isLeaf(root)) {
        code[depth] = L'\0';
        huffmanCodes[root->symbol] = wcsdup(code);
        return;
    }

    code[depth] = L'0';
    buildCodes(root->left, code, depth + 1);
    code[depth] = L'1';
    buildCodes(root->right, code, depth + 1);
}

// Кодирование текста
void encodeText(const wchar_t* input, FILE* outEncoded) {
    for (int i = 0; input[i] != L'\0'; ++i) {
        if (huffmanCodes[input[i]]) {
            fputws(huffmanCodes[input[i]], outEncoded);
        }
    }
}

// Декодирование текста
void decodeText(const wchar_t* encoded, HuffmanNode* root, FILE* outDecoded) {
    HuffmanNode* current = root;
    for (int i = 0; encoded[i] != L'\0'; ++i) {
        if (encoded[i] == L'0') current = current->left;
        else if (encoded[i] == L'1') current = current->right;

        if (isLeaf(current)) {
            fputwc(current->symbol, outDecoded);
            current = root;
        }
    }
}

// Освобождение дерева
void freeTree(HuffmanNode* root) {
    if (!root) return;
    freeTree(root->left);
    freeTree(root->right);
    free(root);
}

int main() {
    // Установка локали
    setlocale(LC_ALL, "");

#ifdef _WIN32
    _setmode(_fileno(stdout), _O_U8TEXT);
    _setmode(_fileno(stdin), _O_U8TEXT);
#endif

    // Открываем файлы
    FILE* in = _wfopen(L"input.txt", L"r, ccs=UTF-8");
    FILE* out = _wfopen(L"output.txt", L"w, ccs=UTF-8");

    if (!in || !out) {
        fwprintf(stderr, L"Ошибка открытия файлов.\n");
        return 1;
    }

    // Считываем весь входной текст
    wchar_t* inputText = (wchar_t*)malloc(sizeof(wchar_t) * 100000);
    wint_t ch;
    int length = 0;
    int freq[MAX_SYMBOLS] = {0};

    while ((ch = fgetwc(in)) != WEOF) {
        inputText[length++] = (wchar_t)ch;
        freq[(wchar_t)ch]++;
    }
    inputText[length] = L'\0';

    // Строим дерево и таблицу кодов
    HuffmanNode* root = buildHuffmanTree(freq);
    wchar_t tempCode[MAX_TREE_HT];
    buildCodes(root, tempCode, 0);

    // Выводим таблицу кодов
    fwprintf(out, L"--- Коды символов ---\n");
    for (int i = 0; i < MAX_SYMBOLS; ++i) {
        if (huffmanCodes[i]) {
            if (i == L'\n') fwprintf(out, L"'\\n': %ls\n", huffmanCodes[i]);
            else fwprintf(out, L"'%lc': %ls\n", (wchar_t)i, huffmanCodes[i]);
        }
    }

    // Кодируем текст
    fwprintf(out, L"\n--- Закодированный текст ---\n");
    encodeText(inputText, out);

    // Строим строку закодированного текста (для обратного декодирования)
    wchar_t* encodedText = (wchar_t*)malloc(sizeof(wchar_t) * 100000);
    encodedText[0] = L'\0';
    for (int i = 0; inputText[i] != L'\0'; ++i) {
        wcscat(encodedText, huffmanCodes[inputText[i]]);
    }

    // Декодируем обратно
    fwprintf(out, L"\n\n--- Декодированный текст ---\n");
    decodeText(encodedText, root, out);

    // Освобождаем ресурсы
    for (int i = 0; i < MAX_SYMBOLS; ++i) {
        if (huffmanCodes[i]) free(huffmanCodes[i]);
    }

    free(inputText);
    free(encodedText);
    freeTree(root);

    fclose(in);
    fclose(out);

    return 0;
}
