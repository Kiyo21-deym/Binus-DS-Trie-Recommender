#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#define MAX_STR 100
#define MAX_DATA 200
#define ALPHABET_SIZE 128
#define SCREEN_WIDTH 80

#ifdef _WIN32
    #include <windows.h>
    #define sleep_ms(ms) Sleep(ms)
#else
    #include <unistd.h>
    #define sleep_ms(ms) usleep((ms) * 1000)
#endif

#define COLOR_RESET   "\x1b[0m"
#define COLOR_GREEN   "\x1b[32m"     
#define COLOR_B_GREEN "\x1b[1;32m"   
#define COLOR_WHITE   "\x1b[37m"     
#define COLOR_B_WHITE "\x1b[1;37m"   
#define COLOR_RED     "\x1b[31m"     
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_YELLOW  "\x1b[33m"

/* ============================================================================
   STRUCTURE DEFINITIONS (DATABASE SCHEMAS)
   ============================================================================ */
typedef struct {
    int id; 
    char username[MAX_STR]; 
    char password[MAX_STR];
    char role[MAX_STR]; 
    char status[MAX_STR];
} User;

typedef struct {
    int id; 
    char name[MAX_STR]; 
    char brand[MAX_STR]; 
    char category[MAX_STR];
    double price; 
    int stock; 
    int seller_id; 
    int search_count;
    int sales_count; 
} Product;

typedef struct {
    int user_id; 
    int product_id; 
    int quantity;
} Cart;

typedef struct {
    int user_id; 
    int product_id;
} Wishlist;

typedef struct {
    int order_id; 
    int customer_id; 
    int seller_id; 
    double total_amount;
    char voucher_code[MAX_STR]; 
    char payment_method[MAX_STR];
    char shipping_method[MAX_STR]; 
    char status[MAX_STR]; 
    char order_date[MAX_STR];
} Order;

typedef struct {
    int order_id;
    int product_id;
    int quantity;
    double price_at_purchase;
} OrderItem;

typedef struct {
    char voucher_code[MAX_STR]; 
    int discount_percent;
    double max_discount_amount;
    double min_purchase; 
    char status[MAX_STR];
} Voucher;

typedef struct {
    int review_id; 
    int order_id;
    int product_id; 
    int customer_id;
    int rating; 
    char comment[MAX_STR];
} Review;

typedef struct {
    int product_id;
} RecentView;

typedef struct TrieNode {
    struct TrieNode *children[ALPHABET_SIZE];
    bool is_end_of_word;
    int product_ids[MAX_DATA];
    int product_count;
    int global_search_count;
} TrieNode;

/* ============================================================================
   GLOBAL STATE VARIABLES
   ============================================================================ */
User users[MAX_DATA]; int user_count = 0;
Product products[MAX_DATA]; int product_count = 0;
Cart carts[MAX_DATA]; int cart_count = 0;
Wishlist wishlists[MAX_DATA]; int wishlist_count = 0;
Order orders[MAX_DATA]; int order_count = 0;
OrderItem order_items[MAX_DATA]; int order_item_count = 0;
Voucher vouchers[MAX_DATA]; int voucher_count = 0;
Review reviews[MAX_DATA]; int review_count = 0;

RecentView recents[MAX_DATA]; int recent_count = 0;

int logged_in_user_idx = -1;
char last_search_keyword[MAX_STR] = "Belum ada";

TrieNode *root_product_name = NULL; 
TrieNode *root_brand = NULL; 
TrieNode *root_category = NULL;

/* ============================================================================
   UTILITY & UI STYLING FUNCTIONS
   ============================================================================ */
/**
 * PENGATURAN UI DAN INPUT VALIDASI MANUAL
 * Paragraf ini menjelaskan utilitas pembersihan layar, perataan teks tengah (centered UI), 
 * dan pembacaan string/angka yang aman tanpa ctype.h. Fungsi custom_tolower mengubah 
 * huruf kapital menjadi kecil secara manual menggunakan operasi selisih ASCII ('A' - 'a'). 
 * Input dibaca menggunakan fungsi read_string_safe yang membungkus fgets() dan membersihkan 
 * karakter newline (\n), mengeliminasi resiko infinite loop akibat sisa buffer dari scanf.
 */
void custom_tolower(char *dest, const char *src) {
    int i = 0;
    while (src[i]) {
        if (src[i] >= 'A' && src[i] <= 'Z') {
            dest[i] = src[i] + ('a' - 'A');
        } else {
            dest[i] = src[i];
        }
        i++;
    }
    dest[i] = '\0';
}

void clear_screen() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

void print_centered_color(const char *text, const char *color) {
    int len = strlen(text);
    int padding = (SCREEN_WIDTH - len) / 2;
    if (padding < 0) padding = 0;
    for (int i = 0; i < padding; i++) printf(" ");
    printf("%s%s%s\n", color, text, COLOR_RESET);
}

void print_warm_welcome_bar(const char *username, const char *role) {
    printf("%s================================================================================%s\n", COLOR_GREEN, COLOR_RESET);
    printf("%sHallo, %s! Selamat datang di BLOCKY ELECTRO (%s).%s\n", COLOR_B_GREEN, username, role, COLOR_RESET);
    printf("%sAda yang bisa kami bantu hari ini?%s\n", COLOR_WHITE, COLOR_RESET);
    printf("%s================================================================================%s\n", COLOR_GREEN, COLOR_RESET);
}

void print_ascii_logo() {
    printf("\n");
    print_centered_color("  ____  _     ___   ____ _  ____   __  _____ _     _____ ____ _____ ____   ___  ", COLOR_GREEN);
    print_centered_color(" | __ )| |   / _ \\ / ___| |/ /\\ \\ / / | ____| |   | ____/ ___|_   _|  _ \\ / _ \\ ", COLOR_GREEN);
    print_centered_color(" |  _ \\| |  | | | | |   | ' /  \\ V /  |  _| | |   |  _|| |     | | | |_) | | | |", COLOR_GREEN);
    print_centered_color(" | |_) | |__| |_| | |___| . \\   | |   | |___| |___| |__| |___  | | |  _ <| |_| |", COLOR_GREEN);
    print_centered_color(" |____/|_____\\___/ \\____|_|\\_\\  |_|   |_____|_____|_____\\____| |_| |_| \\_\\\\___/ ", COLOR_GREEN);
    printf("\n");
    print_centered_color(">>> Sesi Interaktif Belanja Berbasis Algoritma Tries <<<", COLOR_B_WHITE);
    printf("\n");
}

void read_string_safe(char *buffer, int max_len) {
    fgets(buffer, max_len, stdin);
    int len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    }
}

int read_int_safe() {
    char buffer[MAX_STR];
    fgets(buffer, sizeof(buffer), stdin);
    int val = 0;
    if (sscanf(buffer, "%d", &val) != 1) {
        return -1;
    }
    return val;
}

double read_double_safe() {
    char buffer[MAX_STR];
    fgets(buffer, sizeof(buffer), stdin);
    double val = 0;
    if (sscanf(buffer, "%lf", &val) != 1) {
        return -1;
    }
    return val;
}

void press_enter_to_continue() {
    printf("\nTekan [ENTER] untuk melanjutkan...");
    char buf[10];
    fgets(buf, sizeof(buf), stdin);
}

void show_loading_screen() {
    const char *status_messages[] = {
        "Connecting to Blocky Electro Database Cluster...",
        "Building Core Trie Tree Data Structure for autocomplete...",
        "Mapping alphabet dimensions [ALPHABET_SIZE: 128]...",
        "Indexing node branches for lightning-fast search...",
        "System configuration initialized successfully!"
    };
    int message_count = 5;

    for (int progress = 0; progress <= 100; progress += 10) {
        clear_screen();
        printf("\n\n\n\n");
        print_centered_color("==========================================================", COLOR_CYAN);
        print_centered_color("              INITIALIZING BLOCKY CORE ENGINE             ", COLOR_B_WHITE);
        print_centered_color("==========================================================", COLOR_CYAN);
        printf("\n\n");

        printf("\t\t       Progress: [");
        int filled_blocks = progress / 4;
        for (int b = 0; b < 25; b++) {
            if (b < filled_blocks) printf("%sK%s", COLOR_GREEN, COLOR_RESET);
            else printf("%s·%s", COLOR_WHITE, COLOR_RESET);
        }
        printf("] %s%d%%%s\n\n", COLOR_B_GREEN, progress, COLOR_RESET);

        int msg_idx = (progress / 25);
        if (msg_idx >= message_count) msg_idx = message_count - 1;
        printf("\t\t       %s? STATUS:%s %s\n", COLOR_YELLOW, COLOR_RESET, status_messages[msg_idx]);
        fflush(stdout);
        sleep_ms(40);
    }
    printf("\n\n");
    print_centered_color(">>>>> ALL SYSTEMS ARE OPERATIONAL [READY] <<<<<", COLOR_B_GREEN);
    press_enter_to_continue();
}

/* ============================================================================
   TRIE DATA STRUCTURE LOGIC
   ============================================================================ */
/**
 * STRUKTUR DATA DAN MANIPULASI TRIE TREE
 * Paragraf ini mendokumentasikan fungsionalitas Trie Tree yang menjadi mesin pencarian utama. 
 * Fungsi create_trie_node mengalokasikan memori dinamis untuk cabang karakter sebesar 128 (ASCII lengkap). 
 * Fungsi insert_trie memasukkan kata kunci ke dalam hierarki pohon secara rekursif/iteratif, 
 * memetakan product_id pada node akhir (is_end_of_word) untuk menghindari duplikasi data. 
 * Tokenisasi kalimat dilakukan secara mandiri lewat insert_trie_tokens memecah spasi frase produk. 
 * Pencarian awalan dilakukan oleh search_trie_prefix yang menelusuri kedalaman karakter dan 
 * memanggil collect_all_words guna mengumpulkan semua ID produk yang berwujud autocomplete-match.
 */
TrieNode *create_trie_node() {
    TrieNode *node = (TrieNode *)malloc(sizeof(TrieNode));
    if (node != NULL) {
        node->is_end_of_word = false; 
        node->product_count = 0; 
        node->global_search_count = 0;
        for (int i = 0; i < ALPHABET_SIZE; i++) node->children[i] = NULL;
    }
    return node;
}

void insert_trie(TrieNode *root, const char *keyword, int product_id) {
    if (root == NULL || keyword == NULL || strlen(keyword) == 0) return;
    char lower_keyword[MAX_STR]; custom_tolower(lower_keyword, keyword);
    TrieNode *current = root; int len = strlen(lower_keyword);
    for (int i = 0; i < len; i++) {
        int index = (int)lower_keyword[i];
        if (index < 0 || index >= ALPHABET_SIZE) continue;
        if (current->children[index] == NULL) current->children[index] = create_trie_node();
        current = current->children[index];
    }
    current->is_end_of_word = true;
    for(int i = 0; i < current->product_count; i++) {
        if(current->product_ids[i] == product_id) return;
    }
    if (current->product_count < MAX_DATA) current->product_ids[current->product_count++] = product_id;
}

void insert_trie_tokens(TrieNode *root, const char *phrase, int product_id) {
    if (root == NULL || phrase == NULL) return;
    char temp[MAX_STR]; strcpy(temp, phrase);
    char *token = strtok(temp, " ");
    while (token != NULL) {
        insert_trie(root, token, product_id);
        token = strtok(NULL, " ");
    }
}

void collect_all_words(TrieNode *node, int *results, int *res_count) {
    if (node == NULL || *res_count >= MAX_DATA) return;
    if (node->is_end_of_word) {
        for (int i = 0; i < node->product_count; i++) {
            bool dup = false;
            for (int j = 0; j < *res_count; j++) { 
                if (results[j] == node->product_ids[i]) { dup = true; break; } 
            }
            if (!dup && *res_count < MAX_DATA) results[(*res_count)++] = node->product_ids[i];
        }
    }
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (node->children[i] != NULL) collect_all_words(node->children[i], results, res_count);
    }
}

int search_trie_prefix(TrieNode *root, const char *prefix, int *matched_ids) {
    if (root == NULL || prefix == NULL || strlen(prefix) == 0) return 0;
    char lower_prefix[MAX_STR]; custom_tolower(lower_prefix, prefix);
    TrieNode *current = root; int len = strlen(lower_prefix);
    for (int i = 0; i < len; i++) {
        int index = (int)lower_prefix[i];
        if (index < 0 || index >= ALPHABET_SIZE || current->children[index] == NULL) return 0;
        current = current->children[index];
    }
    current->global_search_count++;
    int count = 0; collect_all_words(current, matched_ids, &count);
    return count;
}

void free_trie(TrieNode *node) {
    if (node == NULL) return;
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (node->children[i] != NULL) free_trie(node->children[i]);
    }
    free(node);
}

void rebuild_trives() {
    if (root_product_name != NULL) free_trie(root_product_name);
    if (root_brand != NULL) free_trie(root_brand);
    if (root_category != NULL) free_trie(root_category);

    root_product_name = create_trie_node(); 
    root_brand = create_trie_node(); 
    root_category = create_trie_node();
    
    for (int i = 0; i < product_count; i++) {
        insert_trie(root_product_name, products[i].name, products[i].id);
        insert_trie_tokens(root_product_name, products[i].name, products[i].id);
        insert_trie(root_brand, products[i].brand, products[i].id);
        insert_trie(root_category, products[i].category, products[i].id);
    }
}

/* ============================================================================
   CSV DATABASE MANAGEMENT
   ============================================================================ */
/**
 * SISTEM PERSISTENSI DATA CSV
 * Paragraf ini menerangkan manajemen berkas basis data flat-file .csv. Fungsi load_data 
 * bertanggung jawab membuka seluruh berkas eksternal (users, products, carts, wishlists, dll) 
 * saat aplikasi pertama kali dijalankan, membaca baris demi baris melewati struktur sscanf 
 * untuk disalin ke memori array struct global, dilanjutkan dengan membangun ulang pohon Trie. 
 * Sebaliknya, save_data memicu penulisan ulang (overwrite) kondisi array struct terupdate 
 * kembali ke dalam format file CSV konvensional sesaat setelah transaksi atau mutasi status selesai.
 */
void load_data() {
    FILE *f; char line[500];
    user_count = product_count = cart_count = wishlist_count = order_count = order_item_count = voucher_count = review_count = 0;

    if ((f = fopen("users.csv", "r"))) {
        fgets(line, sizeof(line), f); 
        while (fgets(line, sizeof(line), f) && user_count < MAX_DATA) {
            sscanf(line, "%d,%[^,],%[^,],%[^,],%s", &users[user_count].id, users[user_count].username, users[user_count].password, users[user_count].role, users[user_count].status);
            user_count++;
        } fclose(f);
    }
    if ((f = fopen("products.csv", "r"))) {
        fgets(line, sizeof(line), f); 
        while (fgets(line, sizeof(line), f) && product_count < MAX_DATA) {
            products[product_count].sales_count = 0;
            sscanf(line, "%d,%[^,],%[^,],%[^,],%lf,%d,%d,%d", &products[product_count].id, products[product_count].name, products[product_count].brand, products[product_count].category, &products[product_count].price, &products[product_count].stock, &products[product_count].seller_id, &products[product_count].search_count);
            product_count++;
        } fclose(f);
    }
    if ((f = fopen("carts.csv", "r"))) {
        fgets(line, sizeof(line), f); 
        while (fgets(line, sizeof(line), f) && cart_count < MAX_DATA) {
            sscanf(line, "%d,%d,%d", &carts[cart_count].user_id, &carts[cart_count].product_id, &carts[cart_count].quantity);
            cart_count++;
        } fclose(f);
    }
    if ((f = fopen("wishlists.csv", "r"))) {
        fgets(line, sizeof(line), f); 
        while (fgets(line, sizeof(line), f) && wishlist_count < MAX_DATA) {
            sscanf(line, "%d,%d", &wishlists[wishlist_count].user_id, &wishlists[wishlist_count].product_id);
            wishlist_count++;
        } fclose(f);
    }
    if ((f = fopen("orders.csv", "r"))) {
        fgets(line, sizeof(line), f); 
        while (fgets(line, sizeof(line), f) && order_count < MAX_DATA) {
            sscanf(line, "%d,%d,%d,%lf,%[^,],%[^,],%[^,],%[^,],%s", &orders[order_count].order_id, &orders[order_count].customer_id, &orders[order_count].seller_id, &orders[order_count].total_amount, orders[order_count].voucher_code, orders[order_count].payment_method, orders[order_count].shipping_method, orders[order_count].status, orders[order_count].order_date);
            order_count++;
        } fclose(f);
    }
    if ((f = fopen("order_items.csv", "r"))) {
        fgets(line, sizeof(line), f);
        while (fgets(line, sizeof(line), f) && order_item_count < MAX_DATA) {
            sscanf(line, "%d,%d,%d,%lf", &order_items[order_item_count].order_id, &order_items[order_item_count].product_id, &order_items[order_item_count].quantity, &order_items[order_item_count].price_at_purchase);
            for(int i=0; i<product_count; i++) {
                if(products[i].id == order_items[order_item_count].product_id) {
                    products[i].sales_count += order_items[order_item_count].quantity;
                }
            }
            order_item_count++;
        } fclose(f);
    }
    if ((f = fopen("vouchers.csv", "r"))) {
        fgets(line, sizeof(line), f); 
        while (fgets(line, sizeof(line), f) && voucher_count < MAX_DATA) {
            sscanf(line, "%[^,],%d,%lf,%lf,%s", vouchers[voucher_count].voucher_code, &vouchers[voucher_count].discount_percent, &vouchers[voucher_count].max_discount_amount, &vouchers[voucher_count].min_purchase, vouchers[voucher_count].status);
            voucher_count++;
        } fclose(f);
    }
    if ((f = fopen("reviews.csv", "r"))) {
        fgets(line, sizeof(line), f); 
        while (fgets(line, sizeof(line), f) && review_count < MAX_DATA) {
            sscanf(line, "%d,%d,%d,%d,%d,%[^\n\r]", &reviews[review_count].review_id, &reviews[review_count].order_id, &reviews[review_count].product_id, &reviews[review_count].customer_id, &reviews[review_count].rating, reviews[review_count].comment);
            review_count++;
        } fclose(f);
    }
    rebuild_trives();
}

void save_data() {
    FILE *f;
    if ((f = fopen("users.csv", "w"))) {
        fprintf(f, "id,username,password,role,status\n");
        for (int i = 0; i < user_count; i++) fprintf(f, "%d,%s,%s,%s,%s\n", users[i].id, users[i].username, users[i].password, users[i].role, users[i].status);
        fclose(f);
    }
    if ((f = fopen("products.csv", "w"))) {
        fprintf(f, "id,name,brand,category,price,stock,seller_id,search_count\n");
        for (int i = 0; i < product_count; i++) fprintf(f, "%d,%s,%s,%s,%.2f,%d,%d,%d\n", products[i].id, products[i].name, products[i].brand, products[i].category, products[i].price, products[i].stock, products[i].seller_id, products[i].search_count);
        fclose(f);
    }
    if ((f = fopen("carts.csv", "w"))) {
        fprintf(f, "user_id,product_id,quantity\n");
        for (int i = 0; i < cart_count; i++) fprintf(f, "%d,%d,%d\n", carts[i].user_id, carts[i].product_id, carts[i].quantity);
        fclose(f);
    }
    if ((f = fopen("wishlists.csv", "w"))) {
        fprintf(f, "user_id,product_id\n");
        for (int i = 0; i < wishlist_count; i++) fprintf(f, "%d,%d\n", wishlists[i].user_id, wishlists[i].product_id);
        fclose(f);
    }
    if ((f = fopen("orders.csv", "w"))) {
        fprintf(f, "order_id,customer_id,seller_id,total_amount,voucher_code,payment_method,shipping_method,status,order_date\n");
        for (int i = 0; i < order_count; i++) fprintf(f, "%d,%d,%d,%.2f,%s,%s,%s,%s,%s\n", orders[i].order_id, orders[i].customer_id, orders[i].seller_id, orders[i].total_amount, orders[i].voucher_code, orders[i].payment_method, orders[i].shipping_method, orders[i].status, orders[i].order_date);
        fclose(f);
    }
    if ((f = fopen("order_items.csv", "w"))) {
        fprintf(f, "order_id,product_id,quantity,price_at_purchase\n");
        for (int i = 0; i < order_item_count; i++) fprintf(f, "%d,%d,%d,%.2f\n", order_items[i].order_id, order_items[i].product_id, order_items[i].quantity, order_items[i].price_at_purchase);
        fclose(f);
    }
    if ((f = fopen("vouchers.csv", "w"))) {
        fprintf(f, "voucher_code,discount_percent,max_discount_amount,min_purchase,status\n");
        for (int i = 0; i < voucher_count; i++) fprintf(f, "%s,%d,%.2f,%.2f,%s\n", vouchers[i].voucher_code, vouchers[i].discount_percent, vouchers[i].max_discount_amount, vouchers[i].min_purchase, vouchers[i].status);
        fclose(f);
    }
    if ((f = fopen("reviews.csv", "w"))) {
        fprintf(f, "review_id,order_id,product_id,customer_id,rating,comment\n");
        for (int i = 0; i < review_count; i++) fprintf(f, "%d,%d,%d,%d,%d,%s\n", reviews[i].review_id, reviews[i].order_id, reviews[i].product_id, reviews[i].customer_id, reviews[i].rating, reviews[i].comment);
        fclose(f);
    }
}

/* ============================================================================
   CUSTOMER OPERATIONS & INTERFACES
   ============================================================================ */
/**
 * OPERASI BELANJA DAN RIWAYAT PELANGGAN
 * Bagian ini memproses alur kerja bagi pengguna berstatus Customer. Fitur add_to_cart 
 * memeriksa ketersediaan item serupa dalam keranjang belanja untuk diakumulasikan kuantitasnya. 
 * Fungsi manage_cart_menu menyediakan opsi mengubah jumlah barang, menghapus baris item, 
 * hingga gerbang *checkout* yang mendukung penginputan kode voucher aktif, metode kirim (GOSEND/JNE), 
 * pembayaran (GOPAY/BANK), serta memotong stok fisik komoditas toko secara real-time. 
 * Menu pendukung lain seperti pemindahan wishlist menuju cart, pelacakan histori status order 
 * (PENDING/SHIPPED/COMPLETED/CANCELLED), pembuatan ulasan rating kepuasan (1-5 bintang), 
 * serta pencatatan jejak riwayat baru (Recently Viewed) diwadahi secara terpadu di modul ini.
 */
void add_to_cart(int product_id, int qty) {
    int uid = users[logged_in_user_idx].id;
    for (int i = 0; i < cart_count; i++) {
        if (carts[i].user_id == uid && carts[i].product_id == product_id) {
            carts[i].quantity += qty;
            save_data();
            printf("%sKuantitas belanjaan berhasil disesuaikan!%s\n", COLOR_GREEN, COLOR_RESET);
            return;
        }
    }
    if (cart_count < MAX_DATA) {
        carts[cart_count].user_id = uid;
        carts[cart_count].product_id = product_id;
        carts[cart_count].quantity = qty;
        cart_count++; save_data();
        printf("%sProduk sukses masuk ke keranjang belanja anda!%s\n", COLOR_GREEN, COLOR_RESET);
    }
}

void manage_cart_menu() {
    clear_screen();
    int uid = users[logged_in_user_idx].id;
    print_warm_welcome_bar(users[logged_in_user_idx].username, "Keranjang Belanja");
    printf("\n%10s %-30s %-10s %-15s\n", "ID Produk", "Nama Barang", "Jumlah", "Subtotal");
    double total = 0;
    for (int i = 0; i < cart_count; i++) {
        if (carts[i].user_id == uid) {
            for (int j = 0; j < product_count; j++) {
                if (products[j].id == carts[i].product_id) {
                    double sub = products[j].price * carts[i].quantity;
                    total += sub;
                    printf("%10d %-30s %-10d Rp%-15.2f\n", products[j].id, products[j].name, carts[i].quantity, sub);
                }
            }
        }
    }
    printf("--------------------------------------------------------------------------------\n");
    printf("Total Tagihan Sewaktu: Rp%.2f\n", total);
    printf("1. Ubah Kuantitas\n2. Hapus Item\n3. Lanjut Checkout / Buy Now\n4. Kembali\nPilihan: ");
    int opt = read_int_safe();
    if (opt == 1) {
        printf("ID Produk: "); int pid = read_int_safe();
        printf("Kuantitas Baru: "); int nqty = read_int_safe();
        for(int i=0; i<cart_count; i++) {
            if(carts[i].user_id == uid && carts[i].product_id == pid) { carts[i].quantity = nqty; break; }
        }
        save_data();
    } else if (opt == 2) {
        printf("Masukkan ID Produk: "); int pid = read_int_safe();
        for (int i = 0; i < cart_count; i++) {
            if (carts[i].user_id == uid && carts[i].product_id == pid) {
                for (int j = i; j < cart_count - 1; j++) carts[j] = carts[j + 1];
                cart_count--; break;
            }
        }
        save_data();
    } else if (opt == 3) {
        if (total <= 0) { printf("Keranjang kosong!\n"); press_enter_to_continue(); return; }
        printf("Masukkan Kode Voucher (Kosongkan jika tidak ada): "); 
        char vcode[MAX_STR]; read_string_safe(vcode, MAX_STR);
        double disc = 0;
        if (strlen(vcode) > 0) {
            for (int i = 0; i < voucher_count; i++) {
                if (strcmp(vouchers[i].voucher_code, vcode) == 0 && strcmp(vouchers[i].status, "ACTIVE") == 0) {
                    if (total >= vouchers[i].min_purchase) {
                        disc = (vouchers[i].discount_percent / 100.0) * total;
                        if(disc > vouchers[i].max_discount_amount) disc = vouchers[i].max_discount_amount;
                        printf("Voucher Berhasil Dipasang! Potongan: Rp%.2f\n", disc);
                    } else {
                        printf("Total belanja kurang untuk voucher ini.\n");
                    }
                }
            }
        }
        char pmeth[MAX_STR], smeth[MAX_STR];
        printf("Metode Pembayaran (GOPAY/BANK_TRANSFER): "); read_string_safe(pmeth, MAX_STR);
        printf("Metode Pengiriman (GOSEND/JNE_REGULAR): "); read_string_safe(smeth, MAX_STR);
        
        int new_order_id = order_count + 101;
        int active_seller_id = 2; 

        orders[order_count].order_id = new_order_id;
        orders[order_count].customer_id = uid;
        orders[order_count].seller_id = active_seller_id;
        orders[order_count].total_amount = total - disc;
        strcpy(orders[order_count].voucher_code, strlen(vcode) == 0 ? "NONE" : vcode);
        strcpy(orders[order_count].payment_method, pmeth);
        strcpy(orders[order_count].shipping_method, smeth);
        strcpy(orders[order_count].status, "PENDING");
        strcpy(orders[order_count].order_date, "2026-06-10");
        order_count++;

        for (int i = 0; i < cart_count; i++) {
            if (carts[i].user_id == uid) {
                for(int j=0; j<product_count; j++) {
                    if(products[j].id == carts[i].product_id) {
                        order_items[order_item_count].order_id = new_order_id;
                        order_items[order_item_count].product_id = products[j].id;
                        order_items[order_item_count].quantity = carts[i].quantity;
                        order_items[order_item_count].price_at_purchase = products[j].price;
                        order_item_count++;
                        products[j].stock -= carts[i].quantity;
                        products[j].sales_count += carts[i].quantity;
                    }
                }
            }
        }
        for (int i = cart_count - 1; i >= 0; i--) {
            if (carts[i].user_id == uid) {
                for (int j = i; j < cart_count - 1; j++) carts[j] = carts[j + 1];
                cart_count--;
            }
        }
        save_data(); printf("Pesanan Berhasil Diproses dengan ID %d!\n", new_order_id);
        press_enter_to_continue();
    }
}

void add_to_recent_view(int pid) {
    for(int i=0; i<recent_count; i++) {
        if(recents[i].product_id == pid) return;
    }
    if(recent_count < MAX_DATA) {
        recents[recent_count++].product_id = pid;
    }
}

void manage_wishlist_menu() {
    clear_screen();
    int uid = users[logged_in_user_idx].id;
    print_warm_welcome_bar(users[logged_in_user_idx].username, "Favorit & Wishlist");
    for (int i = 0; i < wishlist_count; i++) {
        if (wishlists[i].user_id == uid) {
            for (int j = 0; j < product_count; j++) {
                if (products[j].id == wishlists[i].product_id) {
                    printf("[-] ID: %d | %s - Rp%.2f\n", products[j].id, products[j].name, products[j].price);
                }
            }
        }
    }
    printf("--------------------------------------------------------------------------------\n");
    printf("1. Pindahkan Wishlist ke Keranjang Belanja\n2. Kembali\nPilihan: ");
    int opt = read_int_safe();
    if (opt == 1) {
        printf("Masukkan ID Produk: "); int pid = read_int_safe();
        add_to_cart(pid, 1);
        for (int i = 0; i < wishlist_count; i++) {
            if (wishlists[i].user_id == uid && wishlists[i].product_id == pid) {
                for (int j = i; j < wishlist_count - 1; j++) wishlists[j] = wishlists[j + 1];
                wishlist_count--; break;
            }
        }
        save_data();
    }
}

void view_customer_orders() {
    clear_screen();
    int uid = users[logged_in_user_idx].id;
    print_warm_welcome_bar(users[logged_in_user_idx].username, "Pesanan Saya & Pelacakan");
    for (int i = 0; i < order_count; i++) {
        if (orders[i].customer_id == uid) {
            printf("[Order ID: %d] Total: Rp%.2f | Status Pelacakan: %s | Tanggal: %s\n", orders[i].order_id, orders[i].total_amount, orders[i].status, orders[i].order_date);
        }
    }
    printf("--------------------------------------------------------------------------------\n");
    printf("1. Konfirmasi Barang Diterima\n2. Beri Ulasan & Rating (Review)\n3. Batalkan Pesanan\n4. Lihat Detail Riwayat Pembelian\n5. Kembali\nPilihan: ");
    int opt = read_int_safe();
    if (opt == 1) {
        printf("Masukkan ID Order: "); int oid = read_int_safe();
        for(int i=0; i<order_count; i++) {
            if(orders[i].order_id == oid && orders[i].customer_id == uid) { strcpy(orders[i].status, "COMPLETED"); break; }
        }
        save_data();
    } else if (opt == 2) {
        printf("Masukkan ID Order: "); int oid = read_int_safe();
        printf("Masukkan ID Produk: "); int pid = read_int_safe();
        printf("Rating (1-5): "); int rate = read_int_safe();
        printf("Komentar: "); char comm[MAX_STR]; read_string_safe(comm, MAX_STR);
        if (review_count < MAX_DATA) {
            reviews[review_count].review_id = review_count + 1;
            reviews[review_count].order_id = oid;
            reviews[review_count].product_id = pid;
            reviews[review_count].customer_id = uid;
            reviews[review_count].rating = rate;
            strcpy(reviews[review_count].comment, comm);
            review_count++; save_data();
            printf("Ulasan Anda berhasil disimpan!\n");
            press_enter_to_continue();
        }
    } else if (opt == 3) {
        printf("Masukkan ID Order: "); int oid = read_int_safe();
        for(int i=0; i<order_count; i++) {
            if(orders[i].order_id == oid && orders[i].customer_id == uid && strcmp(orders[i].status, "PENDING") == 0) {
                strcpy(orders[i].status, "CANCELLED"); break;
            }
        }
        save_data();
    } else if (opt == 4) {
        printf("Masukkan ID Order untuk detail: "); int oid = read_int_safe();
        printf("\n--- DETAIL RIWAYAT PEMBELIAN ID %d ---\n", oid);
        for(int i=0; i<order_item_count; i++) {
            if(order_items[i].order_id == oid) {
                printf("Produk ID: %d | Jumlah: %d | Harga Beli: Rp%.2f\n", order_items[i].product_id, order_items[i].quantity, order_items[i].price_at_purchase);
            }
        }
        press_enter_to_continue();
    }
}

void execution_search_menu() {
    clear_screen();
    print_warm_welcome_bar(users[logged_in_user_idx].username, "Discovery & Smart Search Engine");
    
    printf("%s[INFO]%s Pencarian Terakhir Anda: %s\n", COLOR_B_GREEN, COLOR_RESET, last_search_keyword);
    printf("%s[TRENDING]%s Produk Paling Banyak Dicari: ", COLOR_GREEN, COLOR_RESET);
    int top_search_idx = -1; int max_search = -1;
    for (int i = 0; i < product_count; i++) {
        if (products[i].search_count > max_search) { max_search = products[i].search_count; top_search_idx = i; }
    }
    if(top_search_idx != -1) printf("%s (%dx dicari)\n", products[top_search_idx].name, products[top_search_idx].search_count);
    else printf("Belum terdata\n");
    
    printf("\nKategori Kriteria Pencarian Tries:\n");
    printf(" 1. Berdasarkan Nama Produk (Autocomplete)\n 2. Berdasarkan Merek (Brand)\n 3. Berdasarkan Kategori\n 4. Lihat Semua Produk (Product Discovery)\n");
    printf("Pilihan Anda : ");
    int type = read_int_safe();
    
    int matched_ids[MAX_DATA]; int total = 0;
    if (type >= 1 && type <= 3) {
        printf("Masukkan Prefiks Kata Kunci (Autocomplete): ");
        char keyword[MAX_STR]; read_string_safe(keyword, MAX_STR);
        strcpy(last_search_keyword, keyword);
        TrieNode *target_root = (type == 1) ? root_product_name : ((type == 2) ? root_brand : root_category);
        total = search_trie_prefix(target_root, keyword, matched_ids);
    } else {
        total = product_count;
        for (int i = 0; i < product_count; i++) matched_ids[i] = products[i].id;
    }
    
    if (total == 0) { printf("Produk tidak ditemukan dalam sistem Trie.\n"); press_enter_to_continue(); return; }
    
    printf("\nApakah ingin mengurutkan/filter hasil? (0: Tidak, 1: Harga Terendah, 2: Terlaris/Trending Product): ");
    int sort_opt = read_int_safe();
    if (sort_opt == 1) {
        for(int i=0; i<total-1; i++) {
            for(int j=i+1; j<total; j++) {
                int idx_a = matched_ids[i] - 1, idx_b = matched_ids[j] - 1;
                if(products[idx_a].price > products[idx_b].price) { int temp = matched_ids[i]; matched_ids[i] = matched_ids[j]; matched_ids[j] = temp; }
            }
        }
    } else if (sort_opt == 2) {
        for(int i=0; i<total-1; i++) {
            for(int j=i+1; j<total; j++) {
                int idx_a = matched_ids[i] - 1, idx_b = matched_ids[j] - 1;
                if(products[idx_a].sales_count < products[idx_b].sales_count) { int temp = matched_ids[i]; matched_ids[i] = matched_ids[j]; matched_ids[j] = temp; }
            }
        }
    }

    printf("\n--- HASIL TAMPILAN PRODUK INDEKS ---\n");
    for (int i = 0; i < total; i++) {
        int idx = matched_ids[i] - 1;
        products[idx].search_count++;
        printf("[ID: %-2d] %-25s | Brand: %-10s | Harga: Rp%-10.2f | Stok: %-3d | Terjual: %d\n", 
               products[idx].id, products[idx].name, products[idx].brand, products[idx].price, products[idx].stock, products[idx].sales_count);
    }
    
    printf("\nPilihan Tindakan: 1. Beli & Masuk Keranjang\n2. Tambah ke Wishlist\n3. Lihat Detail & Review\n4. Batal\nPilihan: ");
    int act = read_int_safe();
    if (act == 1) {
        printf("ID Produk: "); int pid = read_int_safe();
        printf("Jumlah: "); int qty = read_int_safe();
        add_to_cart(pid, qty);
    } else if (act == 2) {
        printf("ID Produk: "); int pid = read_int_safe();
        if (wishlist_count < MAX_DATA) {
            wishlists[wishlist_count].user_id = users[logged_in_user_idx].id;
            wishlists[wishlist_count].product_id = pid;
            wishlist_count++; save_data();
            printf("Berhasil disimpan dalam katalog wishlist favorit anda!\n");
        }
    } else if (act == 3) {
        printf("Masukkan ID Produk: "); int pid = read_int_safe();
        add_to_recent_view(pid);
        clear_screen(); printf("--- HALAMAN DETIL KOMODITAS DAN REVIEW PELANGGAN ---\n");
        printf("Nama Item : %s\nMerek     : %s\nKategori  : %s\nHarga Pas : Rp%.2f\nStok Sisa : %d Unit\n", products[pid-1].name, products[pid-1].brand, products[pid-1].category, products[pid-1].price, products[pid-1].stock);
        printf("\n[Review Pembeli]:\n"); bool has_review = false;
        for (int i = 0; i < review_count; i++) {
            if (reviews[i].product_id == pid) { printf(" -> Rating [%d/5]: \"%s\"\n", reviews[i].rating, reviews[i].comment); has_review = true; }
        }
        if(!has_review) printf("   Belum ada ulasan untuk komoditas ini.\n");
        press_enter_to_continue();
    }
}

void customer_menu() {
    int choice;
    do {
        clear_screen();
        print_warm_welcome_bar(users[logged_in_user_idx].username, "CUSTOMER MENU");
        
        printf("\n--- RECENTLY VIEWED PRODUCT ---\n");
        if(recent_count == 0) printf("Belum ada produk yang baru-baru ini dilihat.\n");
        for(int i=0; i<recent_count; i++) {
            int p_idx = recents[i].product_id - 1;
            printf(" -> [ID %d] %s\n", products[p_idx].id, products[p_idx].name);
        }

        printf("\n--- ETALASE PRODUK UNGGULAN SAAT INI ---\n");
        for (int i = 0; i < product_count; i++) {
            printf("[ID: %-2d] %-25s | Harga: Rp%-12.2f | Stok: %d\n", products[i].id, products[i].name, products[i].price, products[i].stock);
        }
        printf("--------------------------------------------------------------------------------\n");
        printf("1. Mesin Pencari Smart Search (Tries, Autocomplete, History, Trending)\n");
        printf("2. Manajemen Keranjang Belanja & Checkout (Buy Now)\n");
        printf("3. Pantau Keranjang Wishlist (Favorit)\n");
        printf("4. Riwayat Transaksi Pesanan Saya (Track & Review)\n");
        printf("5. Pengaturan Edit Profil Akun\n");
        printf("%s6. Keluar Sesi Log Out%s\n", COLOR_RED, COLOR_RESET);
        printf("--------------------------------------------------------------------------------\n");
        printf("Pilihan Menu Customer : ");
        choice = read_int_safe();
        
        switch (choice) {
            case 1: execution_search_menu(); break;
            case 2: manage_cart_menu(); break;
            case 3: manage_wishlist_menu(); break;
            case 4: view_customer_orders(); break;
            case 5:
                clear_screen(); printf("--- EDIT PROFIL CUSTOMER ---\n");
                printf("Username Baru: "); read_string_safe(users[logged_in_user_idx].username, MAX_STR);
                printf("Password Baru: "); read_string_safe(users[logged_in_user_idx].password, MAX_STR);
                save_data(); printf("Perubahan Akun Berhasil Ditulis!\n");
                press_enter_to_continue();
                break;
        }
    } while (choice != 6);
    logged_in_user_idx = -1;
}

/* ============================================================================
   SELLER MANAGEMENT MENU
   ============================================================================ */
/**
 * TOKO DAN ANALITIK PENJUALAN SELLER
 * Paragraf ini mencakup fungsionalitas bagi pengguna yang bertindak sebagai Toko/Seller. 
 * Menu utama Seller Dashboard mencakup manipulasi item katalog seperti menambahkan produk 
 * baru, memodifikasi detail barang, menghapus data, serta memperbarui nilai inventori gudang. 
 * Dari sisi pemrosesan transaksi, Seller dibekali menu penerimaan pesanan masuk (Incoming Orders) 
 * untuk merubah status pengiriman (SHIPPED/REJECTED). Bagian terakhir menyajikan grafik teks 
 * analitik penjualan berupa ringkasan akumulasi Total Pendapatan kotor (Total Revenue) dan 
 * status produk terlaris (Best Selling Product) yang dikalkulasi dari berkas order_items.csv.
 */
void seller_menu() {
    int choice; int sid = users[logged_in_user_idx].id;
    do {
        clear_screen(); print_warm_welcome_bar(users[logged_in_user_idx].username, "SELLER DASHBOARD");
        printf("\n1. Tambah Komoditas Produk Baru\n2. Update Info Stok Barang / Edit\n3. Hapus Produk\n4. Manajemen Pesanan Toko (Accept/Ship/Reject)\n5. Sales Analytics & Statistik Toko\n6. Lihat Ulasan Produk Pembeli\n7. Edit Profil Toko\n8. Keluar Log Out\nPilihan: ");
        choice = read_int_safe();
        if (choice == 1) {
            Product p; p.id = product_count > 0 ? products[product_count-1].id + 1 : 1;
            printf("Nama Barang: "); read_string_safe(p.name, MAX_STR);
            printf("Merek Brand: "); read_string_safe(p.brand, MAX_STR);
            printf("Kategori   : "); read_string_safe(p.category, MAX_STR);
            printf("Harga Jual : "); p.price = read_double_safe();
            printf("Jumlah Stok: "); p.stock = read_int_safe();
            p.seller_id = sid; p.search_count = 0; p.sales_count = 0;
            if (product_count < MAX_DATA) { products[product_count++] = p; save_data(); rebuild_trives(); }
        } else if (choice == 2) {
            printf("ID Produk yang diedit: "); int pid = read_int_safe();
            printf("Stok Baru: "); int nstk = read_int_safe();
            for(int i=0; i<product_count; i++) { if(products[i].id == pid && products[i].seller_id == sid) products[i].stock = nstk; }
            save_data();
        } else if (choice == 3) {
            printf("ID Produk yang dihapus: "); int pid = read_int_safe();
            for (int i = 0; i < product_count; i++) {
                if (products[i].id == pid && products[i].seller_id == sid) {
                    for (int j = i; j < product_count - 1; j++) products[j] = products[j + 1];
                    product_count--; break;
                }
            }
            save_data(); rebuild_trives();
        } else if (choice == 4) {
            clear_screen(); printf("--- DAFTAR INCOMING ORDERS TOKO ANDA ---\n");
            for(int i=0; i<order_count; i++) {
                if(orders[i].seller_id == sid) printf("[ID %d] Total: Rp%.2f | Status: %s\n", orders[i].order_id, orders[i].total_amount, orders[i].status);
            }
            printf("\n1. Terima & Kirim Barang (Ship)\n2. Tolak Pesanan (Reject)\n3. Kembali\nPilihan: ");
            int oopt = read_int_safe();
            if(oopt == 1) {
                printf("Masukkan ID Order: "); int oid = read_int_safe();
                for(int i=0; i<order_count; i++) { if(orders[i].order_id == oid) strcpy(orders[i].status, "SHIPPED"); }
                save_data();
            } else if (oopt == 2) {
                printf("Masukkan ID Order: "); int oid = read_int_safe();
                for(int i=0; i<order_count; i++) { if(orders[i].order_id == oid) strcpy(orders[i].status, "REJECTED"); }
                save_data();
            }
        } else if (choice == 5) {
            clear_screen(); printf("--- SALES ANALYTICS DASHBOARD TOKO ---\n");
            double revenue = 0; int sold_qty = 0;
            for(int i=0; i<order_count; i++) {
                if(orders[i].seller_id == sid && strcmp(orders[i].status, "COMPLETED") == 0) revenue += orders[i].total_amount;
            }
            printf("Total Revenue (Pesanan Selesai): Rp%.2f\n", revenue);
            printf("Estimasi Pendapatan Bulan ini (Juni 2026): Rp%.2f\n", revenue);
            int top_p = -1; int max_s = -1;
            for(int i=0; i<product_count; i++) {
                if(products[i].seller_id == sid && products[i].sales_count > max_s) { max_s = products[i].sales_count; top_p = i; }
            }
            if(top_p != -1) printf("Best Selling Product: %s [Terjual %d unit]\n", products[top_p].name, products[top_p].sales_count);
            press_enter_to_continue();
        } else if (choice == 6) {
            clear_screen(); printf("--- TINJAUAN REVIEW BARANG TOKO ---\n");
            for(int i=0; i<review_count; i++) printf("Produk ID: %d | Rating: %d | Ulasan: %s\n", reviews[i].product_id, reviews[i].rating, reviews[i].comment);
            press_enter_to_continue();
        } else if (choice == 7) {
            printf("Ubah Nama Toko / Username: "); read_string_safe(users[logged_in_user_idx].username, MAX_STR);
            save_data();
        }
    } while (choice != 8);
    logged_in_user_idx = -1;
}

/* ============================================================================
   ADMINISTRATOR DASHBOARD CONTROL
   ============================================================================ */
/**
 * MODUL KONTROL UTAMA ADMINISTRATOR
 * Paragraf ini merangkum hak otorisasi dari akun ber-role Admin. Menu Central Admin 
 * bertugas sebagai pemantau lalu lintas ekosistem e-commerce secara menyeluruh. 
 * Opsi penanganan data mencakup pembekuan akun pengguna bermasalah (Ban User/Seller) 
 * lewat pengubahan bendera status menjadi "BANNED", penghapusan akun, pembuatan kupon voucher 
 * diskon promosi berkala, penghapusan produk melanggar aturan, dan manajemen kategori produk. 
 * Admin juga diberikan laporan dasbor analitik makro, menghitung total seluruh perputaran uang 
 * di aplikasi, jumlah transaksi berhasil, pelacakan kata kunci yang paling sering dicari, 
 * hingga identifikasi statistik demografi kuantitas total User & Seller terdaftar.
 */
void admin_menu() {
    int choice;
    do {
        clear_screen(); print_warm_welcome_bar(users[logged_in_user_idx].username, "ADMINISTRATOR CENTRAL");
        printf("\n1. Lihat Seluruh Pengguna Sistem\n2. Blokir Akun Pengguna / Seller (BANNED)\n3. Hapus Akun Pengguna / Seller\n4. Remove Produk Melanggar\n5. Tambah Kategori Baru\n6. Manajemen Pembuatan Voucher Promo\n7. Analytics & Global Reports Sistem\n8. Keluar Log Out\nPilihan: ");
        choice = read_int_safe();
        if (choice == 1) {
            clear_screen(); printf("--- USER REGISTRATION DATABASE ---\n");
            for(int i=0; i<user_count; i++) printf("ID: %-2d | User: %-15s | Hak: %-10s | Status: %s\n", users[i].id, users[i].username, users[i].role, users[i].status);
            press_enter_to_continue();
        } else if (choice == 2) {
            printf("Username target ban: "); char uname[MAX_STR]; read_string_safe(uname, MAX_STR);
            for(int i=0; i<user_count; i++) { if(strcmp(users[i].username, uname)==0) strcpy(users[i].status, "BANNED"); }
            save_data();
        } else if (choice == 3) {
            printf("Username target delete: "); char uname[MAX_STR]; read_string_safe(uname, MAX_STR);
            for(int i=0; i<user_count; i++) {
                if(strcmp(users[i].username, uname)==0) {
                    for(int j=i; j<user_count-1; j++) users[j] = users[j+1];
                    user_count--; break;
                }
            }
            save_data();
        } else if (choice == 4) {
            printf("ID Produk yang di-remove admin: "); int pid = read_int_safe();
            for (int i = 0; i < product_count; i++) {
                if (products[i].id == pid) {
                    for (int j = i; j < product_count - 1; j++) products[j] = products[j + 1];
                    product_count--; break;
                }
            }
            save_data(); rebuild_trives();
        } else if (choice == 5) {
            printf("Masukkan Nama Kategori Baru: "); char ncat[MAX_STR]; read_string_safe(ncat, MAX_STR);
            printf("Kategori %s berhasil ditambahkan ke pustaka sistem.\n", ncat);
            press_enter_to_continue();
        } else if (choice == 6) {
            Voucher v; printf("Kode Voucher Baru: "); read_string_safe(v.voucher_code, MAX_STR);
            printf("Potongan (%%): "); v.discount_percent = read_int_safe();
            printf("Maksimal Diskon: "); v.max_discount_amount = read_double_safe();
            printf("Minimal Belanja: "); v.min_purchase = read_double_safe();
            strcpy(v.status, "ACTIVE");
            if(voucher_count < MAX_DATA) { vouchers[voucher_count++] = v; save_data(); printf("Voucher Terbikin!\n"); }
            press_enter_to_continue();
        } else if (choice == 7) {
            clear_screen(); printf("--- REPORT GLOBAL MAKRONSOMIS SYSTEM ---\n");
            double g_rev = 0;
            for(int i=0; i<order_count; i++) if(strcmp(orders[i].status, "COMPLETED")==0) g_rev += orders[i].total_amount;
            printf("Total Revenue Ekosistem : Rp%.2f\n", g_rev);
            printf("Total Sukses Transaksi   : %d Transaksi\n", order_count);
            printf("Total Anggota Terdaftar  : %d Entitas\n", user_count);
            press_enter_to_continue();
        }
    } while (choice != 8);
    logged_in_user_idx = -1;
}

/* ============================================================================
   SYSTEM AUTHENTICATION GERBANG UTAMA
   ============================================================================ */
/**
 * SISTEM LOG IN & VALIDASI STATUS AKUN
 * Paragraf ini menerangkan tata cara otentikasi login masuk ke Blocky Electro. 
 * Pengguna menginput kombinasi nama pengguna dan kata kunci rahasia. Loop mencocokkan 
 * string pada database user terdaftar. Bila bendera status berisikan tulisan "BANNED", 
 * gerbang masuk secara instan memblokir hak jalan masuk. Namun apabila kredensial valid, 
 * percabangan role akan melempar pengguna ke dashboard-nya masing-masing (Admin, Seller, Customer).
 */
void execution_login() {
    clear_screen(); 
    print_ascii_logo();
    printf("--- GERBANG SISTEM AUTENTIKASI MASUK ---\n");
    
    char username[MAX_STR], password[MAX_STR];
    printf("Username : "); read_string_safe(username, MAX_STR);
    printf("Password : "); read_string_safe(password, MAX_STR);
    
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0 && strcmp(users[i].password, password) == 0) {
            if (strcmp(users[i].status, "BANNED") == 0) {
                printf("%sAkses Ditolak: Akun Anda ditangguhkan admin.%s\n", COLOR_RED, COLOR_RESET); 
                press_enter_to_continue(); 
                return;
            }
            
            logged_in_user_idx = i;
            
            if (strcmp(users[i].role, "ADMIN") == 0) {
                admin_menu();
            } else if (strcmp(users[i].role, "SELLER") == 0) {
                seller_menu();
            } else {
                show_loading_screen();
                customer_menu();
            }
            return;
        }
    }
    printf("%sIdentitas sandi rahasia salah!%s\n", COLOR_RED, COLOR_RESET); 
    press_enter_to_continue();
}

/* ============================================================================
   MAIN INITIALIZATION TERMINAL PROGRAM ENTRY
   ============================================================================ */
int main() {
    load_data();
    int choice;
    do {
        clear_screen();
        print_ascii_logo();
        printf("--- HALAMAN GERBANG UTAMA INTEGRASI SELLER & CUSTOMER ---\n");
        printf("1. Masuk Sesi Komunitas (Login)\n");
        printf("2. Daftarkan Akun Pembeli Baru (Register)\n");
        printf("%s3. Keluar Tutup Aplikasi%s\n", COLOR_RED, COLOR_RESET);
        printf("--------------------------------------------------------------------------------\n");
        printf("Pilihan Menu : ");
        
        choice = read_int_safe();
        if (choice == 1) {
            execution_login();
        } else if (choice == 2) {
            clear_screen();
            print_ascii_logo();
            printf("--- PENDAFTARAN AKUN CUSTOMER BARU ---\n");
            User u; u.id = user_count > 0 ? users[user_count - 1].id + 1 : 1;
            strcpy(u.role, "CUSTOMER"); strcpy(u.status, "ACTIVE");
            printf("Username Baru : "); read_string_safe(u.username, MAX_STR);
            printf("Password Baru : "); read_string_safe(u.password, MAX_STR);
            
            bool dup = false;
            for(int i = 0; i < user_count; i++) {
                if(strcmp(users[i].username, u.username) == 0) dup = true;
            }
            if(dup) {
                printf("Username sudah terpakai oleh pengguna lain!\n");
                press_enter_to_continue();
                continue;
            }

            if (user_count < MAX_DATA) {
                users[user_count++] = u;
                save_data();
                printf("%sPendaftaran sukses! Silahkan Login.%s\n", COLOR_GREEN, COLOR_RESET);
            }
            press_enter_to_continue();
        }
    } while (choice != 3);
    
    if (root_product_name != NULL) free_trie(root_product_name);
    if (root_brand != NULL) free_trie(root_brand);
    if (root_category != NULL) free_trie(root_category);
    
    printf("\nTerima kasih telah menggunakan BLOCKY ELECTRO. Sampai jumpa kembali!\n");
    return 0;
}
