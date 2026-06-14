#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#define MAX_STR 200
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

#define COLOR_RESET        "\x1b[0m"
#define COLOR_PINE_GREEN   "\x1b[38;2;15;108;56m"
#define COLOR_EMERALD      "\x1b[38;2;40;175;83m"
#define COLOR_WHITE        "\x1b[38;2;240;240;240m"
#define COLOR_B_WHITE      "\x1b[1;37m"
#define COLOR_RED          "\e[0;31m" // Diubah ke RGB Merah asli
#define COLOR_SEA_GREEN    "\x1b[38;2;52;170;98m"
#define COLOR_LIME_GREEN   "\x1b[38;2;170;220;92m"
#define COLOR_DARK_GREEN   "\x1b[38;2;5;70;25m"
#define COLOR_FOREST_GREEN "\x1b[38;2;28;120;47m"

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
    char description[MAX_STR]; 
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
void custom_tolower(char *dest, const char *src) {
    int i = 0;
    while (src[i]) {
        if (src[i] >= 'A' && src[i] <= 'Z') dest[i] = src[i] + ('a' - 'A');
        else dest[i] = src[i];
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
    printf("%s================================================================================%s\n", COLOR_EMERALD, COLOR_RESET);
    printf("%sHalo, %s! | Dasbor: %s%s\n", COLOR_B_WHITE, username, role, COLOR_RESET);
    printf("%s================================================================================%s\n", COLOR_PINE_GREEN, COLOR_RESET);
}

void print_ascii_logo() {
    printf("\n");
    print_centered_color("  ____  _     ___   ____ _  ____   __  _____ _     _____ ____ _____ ____   ___  ", COLOR_EMERALD);
    print_centered_color(" | __ )| |   / _ \\ / ___| |/ /\\ \\ / / | ____| |   | ____/ ___|_   _|  _ \\ / _ \\ ", COLOR_EMERALD);
    print_centered_color(" |  _ \\| |  | | | | |   | ' /  \\ V /  |  _| | |   |  _|| |     | | | |_) | | | |", COLOR_PINE_GREEN);
    print_centered_color(" | |_) | |__| |_| | |___| . \\   | |   | |___| |___| |__| |___  | | |  _ <| |_| |", COLOR_PINE_GREEN);
    print_centered_color(" |____/|_____\\___/ \\____|_|\\_\\  |_|   |_____|_____|_____\\____| |_| |_| \\_\\\\___/ ", COLOR_PINE_GREEN);
    printf("\n");
    print_centered_color(">>> Platform Belanja Elektronik Modern & Terpercaya <<<", COLOR_B_WHITE);
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
    int val = 0;
    while (1) {
        fgets(buffer, sizeof(buffer), stdin);
        if (sscanf(buffer, "%d", &val) == 1) return val;
        printf("%s[!] Input harus berupa angka. Silakan coba lagi:%s ", COLOR_RED, COLOR_RESET);
    }
}

double read_double_safe() {
    char buffer[MAX_STR];
    double val = 0;
    while (1) {
        fgets(buffer, sizeof(buffer), stdin);
        if (sscanf(buffer, "%lf", &val) == 1) return val;
        printf("%s[!] Input harus berupa angka. Silakan coba lagi:%s ", COLOR_RED, COLOR_RESET);
    }
}

void press_enter_to_continue() {
    printf("\nTekan [ENTER] untuk kembali...");
    char buf[10];
    fgets(buf, sizeof(buf), stdin);
}

void show_loading_screen() {
    const char *status_messages[] = {
        "Menghubungkan ke server...", "Memuat data produk...", "Menyiapkan antarmuka...", "Sistem siap digunakan!"
    };
    int message_count = 4;
    for (int progress = 0; progress <= 100; progress += 25) {
        clear_screen();
        printf("\n\n\n\n");
        print_centered_color("==========================================================", COLOR_SEA_GREEN);
        print_centered_color("                 MEMUAT SISTEM BLOCKY                     ", COLOR_B_WHITE);
        print_centered_color("==========================================================", COLOR_SEA_GREEN);
        printf("\n\n");
        printf("\t\t       Progres: [");
        int filled_blocks = progress / 4;
        for (int b = 0; b < 25; b++) {
            if (b < filled_blocks) printf("%s=%s", COLOR_PINE_GREEN, COLOR_RESET);
            else printf("%s-%s", COLOR_WHITE, COLOR_RESET);
        }
        printf("] %s%d%%%s\n\n", COLOR_EMERALD, progress, COLOR_RESET);
        int msg_idx = (progress / 25);
        if (msg_idx >= message_count) msg_idx = message_count - 1;
        printf("\t\t       %s> STATUS:%s %s\n", COLOR_LIME_GREEN, COLOR_RESET, status_messages[msg_idx]);
        fflush(stdout); sleep_ms(60);
    }
    printf("\n");
}

const char* get_username_by_id(int uid) {
    for (int i = 0; i < user_count; i++) {
        if (users[i].id == uid) return users[i].username;
    }
    return "Anonim";
}

void print_stars(int rating) {
    for (int i = 1; i <= 5; i++) {
        if (i <= rating) {
            printf("%s[*]%s", COLOR_LIME_GREEN, COLOR_RESET);
        } else {
            printf("[ ]");
        }
    }
}

/* ============================================================================
   TRIE DATA STRUCTURE LOGIC
   ============================================================================ */
TrieNode *create_trie_node() {
    TrieNode *node = (TrieNode *)malloc(sizeof(TrieNode));
    if (node != NULL) {
        node->is_end_of_word = false; node->product_count = 0; node->global_search_count = 0;
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
            for (int j = 0; j < *res_count; j++) { if (results[j] == node->product_ids[i]) { dup = true; break; } }
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

void rebuild_tries() {
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
    
    if (user_count == 0) {
        users[0].id = 1; strcpy(users[0].username, "admin"); strcpy(users[0].password, "admin123");
        strcpy(users[0].role, "ADMIN"); strcpy(users[0].status, "ACTIVE"); user_count++;
    }

    if ((f = fopen("products.csv", "r"))) {
        fgets(line, sizeof(line), f); 
        while (fgets(line, sizeof(line), f) && product_count < MAX_DATA) {
            products[product_count].sales_count = 0;
            strcpy(products[product_count].description, "Tidak ada deskripsi."); 
            sscanf(line, "%d,%[^,],%[^,],%[^,],%lf,%d,%d,%d,%[^\n\r]", 
                &products[product_count].id, products[product_count].name, products[product_count].brand, 
                products[product_count].category, &products[product_count].price, &products[product_count].stock, 
                &products[product_count].seller_id, &products[product_count].search_count, products[product_count].description);
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
                if(products[i].id == order_items[order_item_count].product_id) products[i].sales_count += order_items[order_item_count].quantity;
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
    rebuild_tries();
}

void save_data() {
    FILE *f;
    if ((f = fopen("users.csv", "w"))) {
        fprintf(f, "id,username,password,role,status\n");
        for (int i = 0; i < user_count; i++) fprintf(f, "%d,%s,%s,%s,%s\n", users[i].id, users[i].username, users[i].password, users[i].role, users[i].status);
        fclose(f);
    }
    if ((f = fopen("products.csv", "w"))) {
        fprintf(f, "id,name,brand,category,price,stock,seller_id,search_count,description\n");
        for (int i = 0; i < product_count; i++) fprintf(f, "%d,%s,%s,%s,%.2f,%d,%d,%d,%s\n", products[i].id, products[i].name, products[i].brand, products[i].category, products[i].price, products[i].stock, products[i].seller_id, products[i].search_count, products[i].description);
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
void view_filtered_reviews(int pid) {
    int target_rating;
    do {
        printf("\nIngin melihat ulasan dengan rating berapa? (1/2/3/4/5) atau 0 untuk Batal: ");
        target_rating = read_int_safe();
        if (target_rating == 0) return; 
        if (target_rating < 1 || target_rating > 5) printf("%s[!] Input tidak valid. Masukkan angka 1 hingga 5.%s\n", COLOR_RED, COLOR_RESET);
    } while (target_rating < 1 || target_rating > 5);

    printf("\n--- Ulasan Bintang %d ---\n", target_rating);
    int found = 0;
    for (int i = 0; i < review_count; i++) {
        if (reviews[i].product_id == pid && reviews[i].rating == target_rating) {
            print_stars(reviews[i].rating);
            printf(" - %s%s%s\n", COLOR_SEA_GREEN, get_username_by_id(reviews[i].customer_id), COLOR_RESET);
            printf("\"%s\"\n\n", reviews[i].comment);
            found++;
        }
    }
    if (found == 0) printf("%s[!] Tidak ada ulasan dengan rating %d bintang.%s\n\n", COLOR_RED, target_rating, COLOR_RESET);
}

void display_product_details(int p_idx) {
    Product p = products[p_idx];
    printf("\n--- DETAIL PRODUK ---\n");
    printf("Nama      : %s\n", p.name);
    printf("Merek     : %s\n", p.brand);
    printf("Kategori  : %s\n", p.category);
    printf("Harga     : Rp%.2f\n", p.price);
    printf("Stok      : %d Unit\n", p.stock);
    printf("Deskripsi : %s\n", p.description);
    
    printf("\n[Ulasan Pembeli]:\n");
    
    int prod_rev_count = 0;
    int rev_indices[MAX_DATA];
    for (int i = 0; i < review_count; i++) {
        if (reviews[i].product_id == p.id) rev_indices[prod_rev_count++] = i;
    }

    if (prod_rev_count == 0) {
        printf("%s[!] Belum ada ulasan untuk produk ini.%s\n", COLOR_LIME_GREEN, COLOR_RESET);
    } else {
        int display_limit = (prod_rev_count > 5) ? 5 : prod_rev_count;
        for (int i = 0; i < display_limit; i++) {
            int r_idx = rev_indices[i];
            print_stars(reviews[r_idx].rating);
            printf(" - %s%s%s\n", COLOR_SEA_GREEN, get_username_by_id(reviews[r_idx].customer_id), COLOR_RESET);
            printf("\"%s\"\n\n", reviews[r_idx].comment);
        }

        if (prod_rev_count > 5) {
            printf("... dan %d ulasan lainnya.\n", prod_rev_count - 5);
            int choice;
            do {
                printf("\nApakah Anda ingin melihat ulasan lainnya?\n");
                printf("1. Ya, lihat berdasarkan rating\n2. Tidak, kembali\nPilihan: ");
                choice = read_int_safe();
                if (choice == 1) { view_filtered_reviews(p.id); break; } 
                else if (choice == 2) { break; } 
                else { printf("%s[!] Pilihan tidak valid. Silakan masukkan 1 atau 2.%s\n", COLOR_RED, COLOR_RESET); }
            } while (choice != 1 && choice != 2);
        }
    }
    press_enter_to_continue();
}

void add_to_cart(int product_id, int qty) {
    int uid = users[logged_in_user_idx].id;
    for (int i = 0; i < cart_count; i++) {
        if (carts[i].user_id == uid && carts[i].product_id == product_id) {
            carts[i].quantity += qty; save_data();
            printf("%sKuantitas belanjaan berhasil disesuaikan!%s\n", COLOR_PINE_GREEN, COLOR_RESET); return;
        }
    }
    if (cart_count < MAX_DATA) {
        carts[cart_count].user_id = uid; carts[cart_count].product_id = product_id; carts[cart_count].quantity = qty;
        cart_count++; save_data();
        printf("%sProduk sukses masuk ke keranjang belanja Anda!%s\n", COLOR_PINE_GREEN, COLOR_RESET);
    }
}

void manage_cart_menu() {
    clear_screen(); int uid = users[logged_in_user_idx].id;
    print_warm_welcome_bar(users[logged_in_user_idx].username, "Keranjang Belanja");
    printf("[INFO] Anda dapat mengubah jumlah barang, menghapus item, atau melanjut ke checkout.\n\n");
    
    printf("%s+------------+-----------------------------------+------------+------------------+%s\n", COLOR_SEA_GREEN, COLOR_RESET);
    printf("%s| %-10s | %-33s | %-10s | %-16s |%s\n", COLOR_SEA_GREEN, "ID Produk", "Nama Barang", "Jumlah", "Subtotal", COLOR_RESET);
    printf("%s+------------+-----------------------------------+------------+------------------+%s\n", COLOR_SEA_GREEN, COLOR_RESET);
    
    double total = 0;
    int items_found = 0;
    for (int i = 0; i < cart_count; i++) {
        if (carts[i].user_id == uid) {
            for (int j = 0; j < product_count; j++) {
                if (products[j].id == carts[i].product_id) {
                    double sub = products[j].price * carts[i].quantity; total += sub;
                    printf("%s| %-10d | %-33.33s | %-10d | Rp%-14.2f |%s\n", COLOR_SEA_GREEN, products[j].id, products[j].name, carts[i].quantity, sub, COLOR_RESET);
                    items_found++;
                }
            }
        }
    }
    
    if (items_found == 0) {
        printf("%s| %-76s |%s\n", COLOR_SEA_GREEN, "Keranjang Anda masih kosong.", COLOR_RESET);
    }
    printf("%s+------------+-----------------------------------+------------+------------------+%s\n", COLOR_SEA_GREEN, COLOR_RESET);
    printf("Total Tagihan: %sRp%.2f%s\n\n", COLOR_LIME_GREEN, total, COLOR_RESET);
    
    printf("1. Ubah Kuantitas\n2. Hapus Item\n3. Lanjut Checkout\n4. Kembali\nPilihan: ");
    int opt = read_int_safe();
    if (opt == 1) {
        printf("ID Produk: "); int pid = read_int_safe();
        printf("Kuantitas Baru: "); int nqty = read_int_safe();
        for(int i=0; i<cart_count; i++) { if(carts[i].user_id == uid && carts[i].product_id == pid) { carts[i].quantity = nqty; break; } }
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
        if (total <= 0) { printf("%s[!] Keranjang masih kosong!%s\n", COLOR_RED, COLOR_RESET); press_enter_to_continue(); return; }
        printf("Kode Voucher (Kosongkan jika tidak ada): "); char vcode[MAX_STR]; read_string_safe(vcode, MAX_STR);
        double disc = 0;
        if (strlen(vcode) > 0) {
            bool v_found = false;
            for (int i = 0; i < voucher_count; i++) {
                if (strcmp(vouchers[i].voucher_code, vcode) == 0 && strcmp(vouchers[i].status, "ACTIVE") == 0) {
                    v_found = true;
                    if (total >= vouchers[i].min_purchase) {
                        disc = (vouchers[i].discount_percent / 100.0) * total;
                        if(disc > vouchers[i].max_discount_amount) disc = vouchers[i].max_discount_amount;
                        printf("%sVoucher Berhasil Dipasang! Potongan: Rp%.2f%s\n", COLOR_PINE_GREEN, disc, COLOR_RESET);
                    } else printf("%s[!] Total belanja kurang untuk voucher ini.%s\n", COLOR_RED, COLOR_RESET);
                }
            }
            if (!v_found) printf("%s[!] Kode voucher tidak valid atau sudah kadaluarsa.%s\n", COLOR_RED, COLOR_RESET);
        }
        char pmeth[MAX_STR], smeth[MAX_STR];
        printf("Metode Pembayaran (Contoh: GOPAY/BANK): "); read_string_safe(pmeth, MAX_STR);
        printf("Metode Pengiriman (Contoh: GOSEND/JNE): "); read_string_safe(smeth, MAX_STR);
        
        int new_order_id = order_count + 101; int active_seller_id = 2; 
        orders[order_count].order_id = new_order_id; orders[order_count].customer_id = uid;
        orders[order_count].seller_id = active_seller_id; orders[order_count].total_amount = total - disc;
        strcpy(orders[order_count].voucher_code, strlen(vcode) == 0 ? "NONE" : vcode);
        strcpy(orders[order_count].payment_method, pmeth); strcpy(orders[order_count].shipping_method, smeth);
        strcpy(orders[order_count].status, "PENDING"); strcpy(orders[order_count].order_date, "2026-06-14");
        order_count++;

        for (int i = 0; i < cart_count; i++) {
            if (carts[i].user_id == uid) {
                for(int j=0; j<product_count; j++) {
                    if(products[j].id == carts[i].product_id) {
                        order_items[order_item_count].order_id = new_order_id; order_items[order_item_count].product_id = products[j].id;
                        order_items[order_item_count].quantity = carts[i].quantity; order_items[order_item_count].price_at_purchase = products[j].price;
                        order_item_count++; products[j].stock -= carts[i].quantity; products[j].sales_count += carts[i].quantity;
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
        save_data(); printf("%sPesanan Berhasil Diproses dengan ID %d!%s\n", COLOR_PINE_GREEN, new_order_id, COLOR_RESET);
        press_enter_to_continue();
    }
}

void add_to_recent_view(int pid) {
    for(int i=0; i<recent_count; i++) { if(recents[i].product_id == pid) return; }
    if(recent_count < MAX_DATA) recents[recent_count++].product_id = pid;
}

void manage_wishlist_menu() {
    clear_screen(); int uid = users[logged_in_user_idx].id;
    print_warm_welcome_bar(users[logged_in_user_idx].username, "Wishlist Favorit");
    printf("[INFO] Daftar produk yang Anda tandai. Bisa dipindahkan ke keranjang.\n\n");
    
    printf("%s+------------+-----------------------------------+------------------+%s\n", COLOR_SEA_GREEN, COLOR_RESET);
    printf("%s| %-10s | %-33s | %-16s |%s\n", COLOR_SEA_GREEN, "ID Produk", "Nama Barang", "Harga", COLOR_RESET);
    printf("%s+------------+-----------------------------------+------------------+%s\n", COLOR_SEA_GREEN, COLOR_RESET);
    
    int items_found = 0;
    for (int i = 0; i < wishlist_count; i++) {
        if (wishlists[i].user_id == uid) {
            for (int j = 0; j < product_count; j++) {
                if (products[j].id == wishlists[i].product_id) {
                    printf("%s| %-10d | %-33.33s | Rp%-14.2f |%s\n", COLOR_SEA_GREEN, products[j].id, products[j].name, products[j].price, COLOR_RESET);
                    items_found++;
                }
            }
        }
    }
    
    if (items_found == 0) {
        printf("%s| %-64s |%s\n", COLOR_SEA_GREEN, "Wishlist Anda masih kosong.", COLOR_RESET);
    }
    printf("%s+------------+-----------------------------------+------------------+%s\n\n", COLOR_SEA_GREEN, COLOR_RESET);
    
    printf("1. Pindah ke Keranjang\n2. Kembali\nPilihan: ");
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
    clear_screen(); int uid = users[logged_in_user_idx].id;
    print_warm_welcome_bar(users[logged_in_user_idx].username, "Riwayat Pesanan");
    printf("[INFO] Konfirmasi barang diterima, batalkan pesanan, atau berikan ulasan.\n\n");
    
    printf("%s+----------+-----------------+-------------+-------------+%s\n", COLOR_SEA_GREEN, COLOR_RESET);
    printf("%s| %-8s | %-15s | %-11s | %-11s |%s\n", COLOR_SEA_GREEN, "Order ID", "Total Harga", "Status", "Tanggal", COLOR_RESET);
    printf("%s+----------+-----------------+-------------+-------------+%s\n", COLOR_SEA_GREEN, COLOR_RESET);
    
    int items_found = 0;
    for (int i = 0; i < order_count; i++) {
        if (orders[i].customer_id == uid) {
            printf("%s| %-8d | Rp%-13.2f | %-11.11s | %-11.11s |%s\n", COLOR_SEA_GREEN, orders[i].order_id, orders[i].total_amount, orders[i].status, orders[i].order_date, COLOR_RESET);
            items_found++;
        }
    }
    
    if (items_found == 0) {
         printf("%s| %-53s |%s\n", COLOR_SEA_GREEN, "Belum ada riwayat pesanan.", COLOR_RESET);
    }
    printf("%s+----------+-----------------+-------------+-------------+%s\n\n", COLOR_SEA_GREEN, COLOR_RESET);
    
    printf("1. Konfirmasi Diterima\n2. Beri Ulasan\n3. Batalkan Pesanan\n4. Detail Pesanan\n5. Kembali\nPilihan: ");
    int opt = read_int_safe();
    if (opt == 1) {
        printf("Masukkan ID Order: "); int oid = read_int_safe();
        for(int i=0; i<order_count; i++) { if(orders[i].order_id == oid && orders[i].customer_id == uid) { strcpy(orders[i].status, "COMPLETED"); break; } }
        save_data();
    } else if (opt == 2) {
        printf("ID Order: "); int oid = read_int_safe();
        printf("ID Produk: "); int pid = read_int_safe();
        int rate;
        do { printf("Rating (1-5): "); rate = read_int_safe(); 
             if(rate < 1 || rate > 5) printf("%s[!] Input tidak valid. Masukkan angka 1 hingga 5.%s\n", COLOR_RED, COLOR_RESET);
        } while(rate < 1 || rate > 5);
        printf("Komentar: "); char comm[MAX_STR]; read_string_safe(comm, MAX_STR);
        if (review_count < MAX_DATA) {
            reviews[review_count].review_id = review_count + 1; reviews[review_count].order_id = oid;
            reviews[review_count].product_id = pid; reviews[review_count].customer_id = uid;
            reviews[review_count].rating = rate; strcpy(reviews[review_count].comment, comm);
            review_count++; save_data();
            printf("%sUlasan berhasil disimpan!%s\n", COLOR_PINE_GREEN, COLOR_RESET);
            printf("Rating Anda: ");
            for (int s = 1; s <= 5; s++) {
                if (s <= rate) printf("[*]");
                else printf("[ ]");
            }
            printf("\n");
            press_enter_to_continue();
        }
    } else if (opt == 3) {
        printf("Masukkan ID Order: "); int oid = read_int_safe();
        for(int i=0; i<order_count; i++) {
            if(orders[i].order_id == oid && orders[i].customer_id == uid && strcmp(orders[i].status, "PENDING") == 0) { strcpy(orders[i].status, "CANCELLED"); break; }
        }
        save_data();
    } else if (opt == 4) {
        printf("Masukkan ID Order: "); int oid = read_int_safe();
        printf("\n--- DETAIL PESANAN %d ---\n", oid);
        printf("%s+------------+------------+------------------+%s\n", COLOR_SEA_GREEN, COLOR_RESET);
        printf("%s| %-10s | %-10s | %-16s |%s\n", COLOR_SEA_GREEN, "Produk ID", "Jumlah", "Harga Beli", COLOR_RESET);
        printf("%s+------------+------------+------------------+%s\n", COLOR_SEA_GREEN, COLOR_RESET);
        for(int i=0; i<order_item_count; i++) {
            if(order_items[i].order_id == oid) {
                printf("%s| %-10d | %-10d | Rp%-14.2f |%s\n", COLOR_SEA_GREEN, order_items[i].product_id, order_items[i].quantity, order_items[i].price_at_purchase, COLOR_RESET);
            }
        }
        printf("%s+------------+------------+------------------+%s\n", COLOR_SEA_GREEN, COLOR_RESET);
        press_enter_to_continue();
    }
}

void execution_search_menu() {
    clear_screen();
    print_warm_welcome_bar(users[logged_in_user_idx].username, "Cari Produk");
    printf("[INFO] Gunakan fitur pencarian untuk menemukan barang yang Anda butuhkan.\n\n");
    printf("%s[INFO]%s Pencarian Terakhir Anda: %s\n", COLOR_EMERALD, COLOR_RESET, last_search_keyword);
    printf("%s[TRENDING]%s Paling Banyak Dicari: ", COLOR_PINE_GREEN, COLOR_RESET);
    int top_search_idx = -1; int max_search = -1;
    for (int i = 0; i < product_count; i++) {
        if (products[i].search_count > max_search) { max_search = products[i].search_count; top_search_idx = i; }
    }
    if(top_search_idx != -1) printf("%s (%dx)\n", products[top_search_idx].name, products[top_search_idx].search_count);
    else printf("%s[!] Belum ada data%s\n", COLOR_RED, COLOR_RESET);
    
    int type;
    do {
        printf("\nPilih Kriteria Pencarian:\n");
        printf("1. Nama Produk\n2. Merek/Brand\n3. Kategori\n4. Lihat Semua\nPilihan: ");
        type = read_int_safe();
        if (type < 1 || type > 4) printf("%s[!] Pilihan tidak valid. Silakan masukkan angka 1 hingga 4.%s\n", COLOR_RED, COLOR_RESET);
    } while (type < 1 || type > 4);
    
    int matched_ids[MAX_DATA]; int total = 0;
    if (type >= 1 && type <= 3) {
        printf("Masukkan Kata Kunci: "); char keyword[MAX_STR]; read_string_safe(keyword, MAX_STR);
        strcpy(last_search_keyword, keyword);
        TrieNode *target_root = (type == 1) ? root_product_name : ((type == 2) ? root_brand : root_category);
        total = search_trie_prefix(target_root, keyword, matched_ids);
    } else {
        total = product_count;
        for (int i = 0; i < product_count; i++) matched_ids[i] = products[i].id;
    }
    
    if (total == 0) { printf("%s[!] Produk tidak ditemukan.%s\n", COLOR_RED, COLOR_RESET); press_enter_to_continue(); return; }
    
    printf("\nUrutkan Hasil? (0: Tidak, 1: Harga Terendah, 2: Terlaris): ");
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

    printf("\n--- HASIL PENCARIAN ---\n");
    printf("%s+----+--------------------------------+------------+----------------+------+%s\n", COLOR_SEA_GREEN, COLOR_RESET);
    printf("%s| %-2s | %-30s | %-10s | %-14s | %-4s |%s\n", COLOR_SEA_GREEN, "ID", "Nama Produk", "Brand", "Harga", "Stok", COLOR_RESET);
    printf("%s+----+--------------------------------+------------+----------------+------+%s\n", COLOR_SEA_GREEN, COLOR_RESET);
    for (int i = 0; i < total; i++) {
        int idx = matched_ids[i] - 1; products[idx].search_count++;
        printf("%s| %-2d | %-30.30s | %-10.10s | Rp%-12.2f | %-4d |%s\n", COLOR_SEA_GREEN, products[idx].id, products[idx].name, products[idx].brand, products[idx].price, products[idx].stock, COLOR_RESET);
    }
    printf("%s+----+--------------------------------+------------+----------------+------+%s\n", COLOR_SEA_GREEN, COLOR_RESET);
    
    printf("\nTindakan:\n1. Beli Sekarang\n2. Tambah Wishlist\n3. Detail Produk\n4. Batal\nPilihan: ");
    int act = read_int_safe();
    if (act == 1) {
        printf("ID Produk: "); int pid = read_int_safe();
        printf("Jumlah: "); int qty = read_int_safe(); add_to_cart(pid, qty);
    } else if (act == 2) {
        printf("ID Produk: "); int pid = read_int_safe();
        if (wishlist_count < MAX_DATA) {
            wishlists[wishlist_count].user_id = users[logged_in_user_idx].id; wishlists[wishlist_count].product_id = pid;
            wishlist_count++; save_data(); printf("%sDisimpan ke wishlist!%s\n", COLOR_PINE_GREEN, COLOR_RESET);
        }
    } else if (act == 3) {
        printf("Masukkan ID Produk: "); int pid = read_int_safe();
        add_to_recent_view(pid); clear_screen(); 
        display_product_details(pid - 1);
    }
}

void customer_menu() {
    int choice;
    do {
        clear_screen(); print_warm_welcome_bar(users[logged_in_user_idx].username, "Menu Customer");
        printf("\n--- TERAKHIR DILIHAT ---\n");
        if(recent_count == 0) printf("%s[!] Belum ada aktivitas.%s\n", COLOR_LIME_GREEN, COLOR_RESET);
        for(int i=0; i<recent_count; i++) {
            int p_idx = recents[i].product_id - 1; printf(" -> [ID %d] %s\n", products[p_idx].id, products[p_idx].name);
        }
        
        printf("\n--- PRODUK UNGGULAN ---\n");
        printf("%s+----+--------------------------------+----------------+------+%s\n", COLOR_SEA_GREEN, COLOR_RESET);
        printf("%s| %-2s | %-30s | %-14s | %-4s |%s\n", COLOR_SEA_GREEN, "ID", "Nama Produk", "Harga", "Stok", COLOR_RESET);
        printf("%s+----+--------------------------------+----------------+------+%s\n", COLOR_SEA_GREEN, COLOR_RESET);
        for (int i = 0; i < product_count; i++) {
            printf("%s| %-2d | %-30.30s | Rp%-12.2f | %-4d |%s\n", COLOR_SEA_GREEN, products[i].id, products[i].name, products[i].price, products[i].stock, COLOR_RESET);
        }
        printf("%s+----+--------------------------------+----------------+------+%s\n", COLOR_SEA_GREEN, COLOR_RESET);
        
        printf("1. Cari Produk\n2. Keranjang Belanja\n3. Wishlist Favorit\n4. Riwayat Pesanan\n5. Edit Profil\n%s6. Keluar Sesi%s\n--------------------------------------------------------------------------------\nPilihan Anda: ", COLOR_RED, COLOR_RESET);
        choice = read_int_safe();
        switch (choice) {
            case 1: execution_search_menu(); break;
            case 2: manage_cart_menu(); break;
            case 3: manage_wishlist_menu(); break;
            case 4: view_customer_orders(); break;
            case 5:
                clear_screen(); printf("--- EDIT PROFIL ---\n");
                printf("Username Baru: "); read_string_safe(users[logged_in_user_idx].username, MAX_STR);
                printf("Password Baru: "); read_string_safe(users[logged_in_user_idx].password, MAX_STR);
                save_data(); printf("%sPerubahan Berhasil Disimpan!%s\n", COLOR_PINE_GREEN, COLOR_RESET); press_enter_to_continue();
                break;
        }
    } while (choice != 6);
    logged_in_user_idx = -1;
}

/* ============================================================================
   SELLER MANAGEMENT MENU
   ============================================================================ */
void seller_menu() {
    int choice; int sid = users[logged_in_user_idx].id;
    do {
        clear_screen(); print_warm_welcome_bar(users[logged_in_user_idx].username, "Menu Seller");
        printf("\n1. Tambah Produk\n2. Update Stok\n3. Hapus Produk\n4. Kelola Pesanan\n5. Analitik Toko\n6. Ulasan Pembeli\n7. Edit Profil\n%s8. Keluar Sesi%s\nPilihan Anda: ", COLOR_RED, COLOR_RESET);
        choice = read_int_safe();
        if (choice == 1) {
            printf("\n[INFO] Menambahkan barang baru ke etalase Anda.\n");
            Product p; p.id = product_count > 0 ? products[product_count-1].id + 1 : 1;
            printf("Nama Barang: "); read_string_safe(p.name, MAX_STR);
            printf("Merek Brand: "); read_string_safe(p.brand, MAX_STR);
            printf("Kategori   : "); read_string_safe(p.category, MAX_STR);
            printf("Harga Jual : "); p.price = read_double_safe();
            printf("Jumlah Stok: "); p.stock = read_int_safe();
            printf("Deskripsi  : "); read_string_safe(p.description, MAX_STR);
            p.seller_id = sid; p.search_count = 0; p.sales_count = 0;
            if (product_count < MAX_DATA) { products[product_count++] = p; save_data(); rebuild_tries(); }
        } else if (choice == 2) {
            printf("\n[INFO] Mengubah jumlah barang yang tersedia.\n");
            printf("ID Produk: "); int pid = read_int_safe();
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
            save_data(); rebuild_tries();
        } else if (choice == 4) {
            clear_screen(); printf("--- PESANAN MASUK ---\n");
            printf("%s+----------+-----------------+-------------+%s\n", COLOR_SEA_GREEN, COLOR_RESET);
            printf("%s| %-8s | %-15s | %-11s |%s\n", COLOR_SEA_GREEN, "Order ID", "Total Harga", "Status", COLOR_RESET);
            printf("%s+----------+-----------------+-------------+%s\n", COLOR_SEA_GREEN, COLOR_RESET);
            for(int i=0; i<order_count; i++) { 
                if(orders[i].seller_id == sid) {
                    printf("%s| %-8d | Rp%-13.2f | %-11.11s |%s\n", COLOR_SEA_GREEN, orders[i].order_id, orders[i].total_amount, orders[i].status, COLOR_RESET);
                }
            }
            printf("%s+----------+-----------------+-------------+%s\n", COLOR_SEA_GREEN, COLOR_RESET);
            printf("\n1. Kirim Barang\n2. Tolak Pesanan\n3. Kembali\nPilihan: ");
            int oopt = read_int_safe();
            if(oopt == 1) {
                printf("ID Order: "); int oid = read_int_safe();
                for(int i=0; i<order_count; i++) { if(orders[i].order_id == oid) strcpy(orders[i].status, "SHIPPED"); } save_data();
            } else if (oopt == 2) {
                printf("ID Order: "); int oid = read_int_safe();
                for(int i=0; i<order_count; i++) { if(orders[i].order_id == oid) strcpy(orders[i].status, "REJECTED"); } save_data();
            }
        } else if (choice == 5) {
            clear_screen(); printf("--- ANALITIK TOKO ---\n");
            double revenue = 0;
            for(int i=0; i<order_count; i++) { if(orders[i].seller_id == sid && strcmp(orders[i].status, "COMPLETED") == 0) revenue += orders[i].total_amount; }
            printf("Total Pendapatan (Selesai): Rp%.2f\n", revenue);
            int top_p = -1; int max_s = -1;
            for(int i=0; i<product_count; i++) { if(products[i].seller_id == sid && products[i].sales_count > max_s) { max_s = products[i].sales_count; top_p = i; } }
            if(top_p != -1) printf("Produk Terlaris: %s [%d Terjual]\n", products[top_p].name, products[top_p].sales_count);
            press_enter_to_continue();
        } else if (choice == 6) {
            clear_screen(); printf("--- ULASAN PRODUK ---\n");
            for(int i=0; i<review_count; i++) {
                printf("Produk ID: %d | User: %s | Rating: ", reviews[i].product_id, get_username_by_id(reviews[i].customer_id));
                print_stars(reviews[i].rating); printf(" | Ulasan: %s\n", reviews[i].comment);
            }
            press_enter_to_continue();
        } else if (choice == 7) {
            printf("Nama Baru: "); read_string_safe(users[logged_in_user_idx].username, MAX_STR); save_data();
        }
    } while (choice != 8);
    logged_in_user_idx = -1;
}

/* ============================================================================
   ADMINISTRATOR DASHBOARD CONTROL (FULL ASCII MURNI & BERWARNA)
   ============================================================================ */

void print_admin_section_header(const char *title) {
    int len = strlen(title);
    printf("%s+", COLOR_DARK_GREEN);
    for (int i = 0; i < len + 4; i++) printf("-");
    printf("+%s\n", COLOR_RESET);
    
    printf("%s|  %s%s%s  |%s\n", COLOR_DARK_GREEN, COLOR_FOREST_GREEN, title, COLOR_DARK_GREEN, COLOR_RESET);
    
    printf("%s+", COLOR_FOREST_GREEN);
    for (int i = 0; i < len + 4; i++) printf("-");
    printf("+%s\n\n", COLOR_RESET);
}

void admin_user_management(int mode) { 
    char keyword[MAX_STR];
    char error_msg[MAX_STR] = ""; 
    
    do {
        clear_screen();
        if (mode == 1) {
            print_admin_section_header("MANAJEMEN OTORISASI: BLOKIR PENGGUNA");
        } else {
            print_admin_section_header("MANAJEMEN OTORISASI: HAPUS PENGGUNA");
        }
        
        if (strlen(error_msg) > 0) {
            printf("%s[!] %s%s\n\n", COLOR_RED, error_msg, COLOR_RESET);
            strcpy(error_msg, ""); 
        }

        printf("%sCari (Username/ID/Role) atau tekan [ENTER] untuk lewati: %s", COLOR_LIME_GREEN, COLOR_RESET);
        read_string_safe(keyword, MAX_STR);

        printf("\n%s+-------+-----------------+------------+------------+%s\n", COLOR_SEA_GREEN, COLOR_RESET);
        printf("%s| %-5s | %-15s | %-10s | %-10s |%s\n", COLOR_SEA_GREEN, "ID", "Username", "Role", "Status", COLOR_RESET);
        printf("%s+-------+-----------------+------------+------------+%s\n", COLOR_SEA_GREEN, COLOR_RESET);

        int count = 0;
        for (int i = 0; i < user_count; i++) {
            if (strlen(keyword) == 0 || strstr(users[i].username, keyword) || strstr(users[i].role, keyword)) {
                printf("%s| %s%-5d %s| %s%-15.15s %s| %s%-10.10s %s| %s%-10.10s %s|%s\n", 
                       COLOR_SEA_GREEN, COLOR_LIME_GREEN, users[i].id, COLOR_SEA_GREEN, 
                       COLOR_B_WHITE, users[i].username, COLOR_SEA_GREEN, 
                       COLOR_PINE_GREEN, users[i].role, COLOR_SEA_GREEN, 
                       COLOR_RED, users[i].status, COLOR_SEA_GREEN, COLOR_RESET);
                count++;
            }
        }
        
        if (count == 0) {
            printf("%s| %-54s |%s\n", COLOR_SEA_GREEN, "Tidak ada data pengguna yang cocok dengan pencarian", COLOR_RESET);
        }
        printf("%s+-------+-----------------+------------+------------+%s\n", COLOR_SEA_GREEN, COLOR_RESET);
        printf("%s[0] Kembali / Batal\n", COLOR_RED);
        printf("%sMasukkan ID Pengguna untuk diproses: %s", COLOR_PINE_GREEN, COLOR_RESET);
        int target_id = read_int_safe();
        
        if (target_id == 0) return; 

        bool found = false;
        for (int i = 0; i < user_count; i++) {
            if (users[i].id == target_id) {
                found = true;
                if (mode == 1) {
                    strcpy(users[i].status, "BANNED");
                } else {
                    for (int j = i; j < user_count - 1; j++) {
                        users[j] = users[j + 1];
                    }
                    user_count--;
                }
                save_data();
                printf("\n%s[✔] Tindakan administrasi berhasil diterapkan!%s\n", COLOR_PINE_GREEN, COLOR_RESET);
                press_enter_to_continue();
                return; 
            }
        }

        if (!found) {
            strcpy(error_msg, "ID Pengguna tidak ditemukan dalam sistem database, silakan coba lagi!");
        }

    } while (true); 
}

void admin_manage_categories() {
    int opt;
    char error_msg[MAX_STR] = "";

    do {
        clear_screen();
        print_admin_section_header("STRUKTUR DATA: KELOLA KATEGORI");
        
        printf("%sDaftar Kategori Terdaftar Saat Ini:%s\n", COLOR_SEA_GREEN, COLOR_RESET);
        char seen[MAX_DATA][MAX_STR];
        int seen_count = 0;
        for (int i = 0; i < product_count; i++) {
            bool found = false;
            for (int j = 0; j < seen_count; j++) {
                if (strcmp(seen[j], products[i].category) == 0) { found = true; break; }
            }
            if (!found) {
                strcpy(seen[seen_count++], products[i].category);
                printf("%s  |-- %s%s%s\n", COLOR_SEA_GREEN, COLOR_LIME_GREEN, products[i].category, COLOR_RESET);
            }
        }
        
        printf("%s----------------------------------%s\n", COLOR_SEA_GREEN, COLOR_RESET);
        if (strlen(error_msg) > 0) {
            printf("%s[!] %s%s\n\n", COLOR_RED, error_msg, COLOR_RESET);
            strcpy(error_msg, ""); 
        }

        printf("%s[1] %sTambah Kategori Baru\n", COLOR_SEA_GREEN, COLOR_LIME_GREEN);
        printf("%s[2] Kembali / Batal\n", COLOR_RED);
        printf("%sPilihan Operasi: %s", COLOR_PINE_GREEN, COLOR_RESET);
        opt = read_int_safe();

        if (opt == 1) {
            char new_cat[MAX_STR];
            printf("%sNama Kategori Baru: %s", COLOR_PINE_GREEN, COLOR_RESET); 
            read_string_safe(new_cat, MAX_STR);
            
            if (strlen(new_cat) == 0) {
                strcpy(error_msg, "Nama kategori baru tidak boleh dikosongkan!");
            } else {
                bool exist = false;
                for(int i = 0; i < product_count; i++) {
                    if(strcmp(products[i].category, new_cat) == 0) exist = true;
                }
                
                if (exist) {
                    strcpy(error_msg, "Gagal! Identifikasi kategori tersebut sudah terdaftar.");
                } else {
                    printf("\n%s[✔] Kategori '%s' sukses diregistrasikan ke database.%s\n", COLOR_PINE_GREEN, new_cat, COLOR_RESET);
                    press_enter_to_continue();
                }
            }
        } else if (opt != 2) {
            strcpy(error_msg, "Pilihan instruksi menu tidak valid!");
        }
    } while (opt != 2);
}

void admin_product_management() {
    char error_msg[MAX_STR] = "";
    do {
        clear_screen();
        print_admin_section_header("KATALOG DATA: INVENTARIS PRODUK");
        
        if (strlen(error_msg) > 0) {
            printf("%s[!] %s%s\n\n", COLOR_RED, error_msg, COLOR_RESET);
            strcpy(error_msg, ""); 
        }

        printf("%s+-------+--------------------------------+----------------------+%s\n", COLOR_SEA_GREEN, COLOR_RESET);
        printf("%s| %-5s | %-30s | %-20s |%s\n", COLOR_SEA_GREEN, "ID", "Nama Produk", "Kategori", COLOR_RESET);
        printf("%s+-------+--------------------------------+----------------------+%s\n", COLOR_SEA_GREEN, COLOR_RESET);
        for(int i = 0; i < product_count; i++) {
            printf("%s| %s%-5d %s| %s%-30.30s %s| %s%-20.20s %s|%s\n", 
                   COLOR_SEA_GREEN, COLOR_LIME_GREEN, products[i].id, COLOR_SEA_GREEN, 
                   COLOR_B_WHITE, products[i].name, COLOR_SEA_GREEN, 
                   COLOR_PINE_GREEN, products[i].category, COLOR_SEA_GREEN, COLOR_RESET);
        }
        printf("%s+-------+--------------------------------+----------------------+%s\n", COLOR_SEA_GREEN, COLOR_RESET);
        
        printf("%s[0] %sKembali ke Menu Utama\n", COLOR_RED, COLOR_LIME_GREEN);
        printf("%sMasukkan ID Produk yang ingin dihapus secara permanen: %s", COLOR_PINE_GREEN, COLOR_RESET);
        int pid = read_int_safe();
        if (pid == 0) return;

        bool found = false;
        for (int i = 0; i < product_count; i++) {
            if (products[i].id == pid) {
                found = true;
                for (int j = i; j < product_count - 1; j++) {
                    products[j] = products[j + 1];
                }
                product_count--;
                save_data(); 
                rebuild_tries();
                printf("\n%s[✔] Produk berhasil dihapus dari inventaris sistem!%s\n", COLOR_PINE_GREEN, COLOR_RESET);
                press_enter_to_continue();
                return;
            }
        }
        
        if (!found) {
            strcpy(error_msg, "ID Produk tidak ditemukan dalam sistem katalog belanja!");
        }
    } while (true);
}

void admin_manage_vouchers() {
    int choice;
    char error_msg[MAX_STR] = "";
    do {
        clear_screen();
        print_admin_section_header("PROMO ENGINE: KELOLA VOUCHER BELANJA");
        
        if (strlen(error_msg) > 0) {
            printf("%s[!] %s%s\n\n", COLOR_RED, error_msg, COLOR_RESET);
            strcpy(error_msg, ""); 
        }

        printf("%s+-----------------+------------+------------------+------------------+%s\n", COLOR_SEA_GREEN, COLOR_RESET);
        printf("%s| %-15s | %-10s | %-16s | %-16s |%s\n", COLOR_SEA_GREEN, "Kode Voucher", "Diskon %", "Maks Potongan", "Min Belanja", COLOR_RESET);
        printf("%s+-----------------+------------+------------------+------------------+%s\n", COLOR_SEA_GREEN, COLOR_RESET);
        for (int i = 0; i < voucher_count; i++) {
            printf("%s| %s%-15.15s %s| %s%-10d %s| %sRp%-14.2f %s| %sRp%-14.2f %s|%s\n", 
                   COLOR_SEA_GREEN, COLOR_B_WHITE, vouchers[i].voucher_code, COLOR_SEA_GREEN, 
                   COLOR_LIME_GREEN, vouchers[i].discount_percent, COLOR_SEA_GREEN, 
                   COLOR_PINE_GREEN, vouchers[i].max_discount_amount, COLOR_SEA_GREEN, 
                   COLOR_PINE_GREEN, vouchers[i].min_purchase, COLOR_SEA_GREEN, COLOR_RESET);
        }
        printf("%s+-----------------+------------+------------------+------------------+%s\n", COLOR_SEA_GREEN, COLOR_RESET);
        
        printf("%s[1] %sRilis Voucher Baru\n", COLOR_SEA_GREEN, COLOR_LIME_GREEN);
        printf("%s[2] %sKembali ke Menu Utama\n", COLOR_RED, COLOR_LIME_GREEN);
        printf("%sPilihan Aksi: %s", COLOR_PINE_GREEN, COLOR_RESET);
        choice = read_int_safe();

        if (choice == 1) {
            if (voucher_count >= MAX_DATA) {
                strcpy(error_msg, "Alokasi penyimpanan database Voucher penuh!");
            } else {
                Voucher v;
                printf("%sKode Voucher Baru : %s", COLOR_PINE_GREEN, COLOR_RESET); read_string_safe(v.voucher_code, MAX_STR);
                printf("%sPotongan Diskon (%%): %s", COLOR_PINE_GREEN, COLOR_RESET); v.discount_percent = read_int_safe();
                printf("%sMaksimal Nominal   : Rp", COLOR_PINE_GREEN); v.max_discount_amount = read_double_safe();
                printf("%sMinimal Pembelian  : Rp", COLOR_PINE_GREEN); v.min_purchase = read_double_safe();
                strcpy(v.status, "ACTIVE");
                
                vouchers[voucher_count++] = v;
                save_data();
                printf("\n%s[✔] Kode promo voucher baru berhasil diaktifkan!%s\n", COLOR_PINE_GREEN, COLOR_RESET);
                press_enter_to_continue();
            }
        } else if (choice != 2) {
            strcpy(error_msg, "Opsi yang dimasukkan di luar jangkauan pilihan promo!");
        }
    } while (choice != 2);
}

void admin_menu() {
    int choice;
    char error_msg[MAX_STR] = ""; 

    do {
        clear_screen();
        print_warm_welcome_bar(users[logged_in_user_idx].username, "Pusat Kontrol Administrator");
        
        if (strlen(error_msg) > 0) {
            printf("%s[!] %s%s\n\n", COLOR_RED, error_msg, COLOR_RESET);
            strcpy(error_msg, ""); 
        }

        printf("%s[1] %sBlokir Akses Pengguna\n", COLOR_SEA_GREEN, COLOR_LIME_GREEN);
        printf("%s[2] %sHapus Data Pengguna\n", COLOR_SEA_GREEN, COLOR_LIME_GREEN);
        printf("%s[3] %sHapus Produk Katalog\n", COLOR_SEA_GREEN, COLOR_LIME_GREEN);
        printf("%s[4] %sKelola Kategori Produk\n", COLOR_SEA_GREEN, COLOR_LIME_GREEN);
        printf("%s[5] %sPenerbitan Promo Voucher\n", COLOR_SEA_GREEN, COLOR_LIME_GREEN);
        printf("%s[6] %sAkses Laporan Finansial Sistem\n", COLOR_SEA_GREEN, COLOR_LIME_GREEN);
        printf("%s[7] Keluar Sesi (Logout System)\n", COLOR_RED, COLOR_RESET);
        printf("%sMasukkan Pilihan Operasi: %s", COLOR_PINE_GREEN, COLOR_RESET);
        choice = read_int_safe();
        
        switch(choice) {
            case 1: admin_user_management(1); break;
            case 2: admin_user_management(2); break;
            case 3: admin_product_management(); break;
            case 4: admin_manage_categories(); break;
            case 5: admin_manage_vouchers(); break;
            case 6: {
                clear_screen(); 
                print_admin_section_header("BUSINESS REPORT: FINANCIAL OVERVIEW");
                double g_rev = 0;
                for(int i = 0; i < order_count; i++) {
                    if(strcmp(orders[i].status, "COMPLETED") == 0) {
                        g_rev += orders[i].total_amount;
                    }
                }
                printf("%s+------------------------------------------------------------+%s\n", COLOR_SEA_GREEN, COLOR_RESET);
                printf("%s| %sTotal Akumulasi Transaksi  : %sRp%-27.2f%s |\n", COLOR_SEA_GREEN, COLOR_LIME_GREEN, COLOR_PINE_GREEN, g_rev, COLOR_SEA_GREEN);
                printf("%s| %sTotal Kuantitas Pesanan    : %s%-29d%s |\n", COLOR_SEA_GREEN, COLOR_LIME_GREEN, COLOR_PINE_GREEN, order_count, COLOR_SEA_GREEN);
                printf("%s| %sTotal Registrasi Pengguna  : %s%-29d%s |\n", COLOR_SEA_GREEN, COLOR_LIME_GREEN, COLOR_PINE_GREEN, user_count, COLOR_SEA_GREEN);
                printf("%s+------------------------------------------------------------+%s\n\n", COLOR_SEA_GREEN, COLOR_RESET);
                press_enter_to_continue();
                break;
            }
            case 7: 
                break;
            default:
                strcpy(error_msg, "Instruksi kode operasional tidak valid, mohon teliti kembali!"); 
                break;
        }
    } while (choice != 7);
    logged_in_user_idx = -1;
}

/* ============================================================================
   SYSTEM AUTHENTICATION GERBANG UTAMA
   ============================================================================ */
void execution_login() {
    clear_screen(); print_ascii_logo(); printf("\n");
    print_centered_color("--- OTENTIKASI PENGGUNA ---", COLOR_B_WHITE); printf("\n");
    char username[MAX_STR], password[MAX_STR];
    printf("                       Username : "); read_string_safe(username, MAX_STR);
    printf("                       Password : "); read_string_safe(password, MAX_STR);
    
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0 && strcmp(users[i].password, password) == 0) {
            if (strcmp(users[i].status, "BANNED") == 0) {
                printf("\n%s[!] Akses Ditolak: Akun Anda ditangguhkan.%s\n", COLOR_RED, COLOR_RESET); 
                press_enter_to_continue(); return;
            }
            logged_in_user_idx = i;
            if (strcmp(users[i].role, "ADMIN") == 0) admin_menu();
            else if (strcmp(users[i].role, "SELLER") == 0) seller_menu();
            else { show_loading_screen(); customer_menu(); }
            return;
        }
    }
    printf("\n%s[!] Kredensial tidak ditemukan atau salah!%s\n", COLOR_RED, COLOR_RESET); press_enter_to_continue();
}

/* ============================================================================
   MAIN INITIALIZATION TERMINAL PROGRAM ENTRY
   ============================================================================ */
int main() {
    load_data(); int choice;
    do {
        clear_screen(); print_ascii_logo(); printf("\n");
        print_centered_color("Silakan pilih opsi navigasi berikut:", COLOR_WHITE); printf("\n");
        printf("                             %s1. Masuk Sesi%s\n", COLOR_PINE_GREEN, COLOR_RESET);
        printf("                             %s2. Daftar Akun%s\n", COLOR_PINE_GREEN, COLOR_RESET);
        printf("                             %s3. Keluar Aplikasi%s\n", COLOR_RED, COLOR_RESET);
        printf("\n"); print_centered_color("==========================================================", COLOR_PINE_GREEN);
        printf("Pilihan Anda : "); choice = read_int_safe();
        if (choice == 1) execution_login();
        else if (choice == 2) {
            clear_screen(); print_ascii_logo(); printf("\n");
            print_centered_color("--- REGISTRASI AKUN BARU ---", COLOR_B_WHITE); printf("\n");
            User u; u.id = user_count > 0 ? users[user_count - 1].id + 1 : 1;
            strcpy(u.role, "CUSTOMER"); strcpy(u.status, "ACTIVE");
            printf("Username Baru : "); read_string_safe(u.username, MAX_STR);
            printf("Password Baru : "); read_string_safe(u.password, MAX_STR);
            bool dup = false;
            for(int i = 0; i < user_count; i++) { if(strcmp(users[i].username, u.username) == 0) dup = true; }
            if(dup) { printf("\n%s[!] Username sudah terpakai! Coba yang lain.%s\n", COLOR_RED, COLOR_RESET); press_enter_to_continue(); continue; }
            if (user_count < MAX_DATA) { users[user_count++] = u; save_data(); printf("\n%sPendaftaran berhasil! Silakan masuk ke sesi Anda.%s\n", COLOR_PINE_GREEN, COLOR_RESET); }
            press_enter_to_continue();
        }
    } while (choice != 3);
    
    if (root_product_name != NULL) free_trie(root_product_name);
    if (root_brand != NULL) free_trie(root_brand);
    if (root_category != NULL) free_trie(root_category);
    printf("\nTerima kasih telah menggunakan layanan kami. Sampai jumpa!\n");
    return 0;
}