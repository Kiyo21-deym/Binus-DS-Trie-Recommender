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
#define COLOR_RED          "\x1b[31m"
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
    char role[20];
    char status[20];
    char full_name[MAX_STR];
    char birth_info[MAX_STR];
    char address[MAX_STR];
    char email[MAX_STR];
    char phone[MAX_STR];
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
    FILE *f; char line[1000]; // Ukuran buffer diperbesar agar muat satu baris panjang
    user_count = product_count = cart_count = wishlist_count = order_count = order_item_count = voucher_count = review_count = 0;

    if ((f = fopen("users.csv", "r"))) {
        fgets(line, sizeof(line), f); // Membaca header CSV
        while (fgets(line, sizeof(line), f) && user_count < MAX_DATA) {
            // Membaca 10 parameter user secara lengkap dan berurutan sesuai struct
            int parsed = sscanf(line, "%d,%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^\n\r]", 
                &users[user_count].id, 
                users[user_count].username, 
                users[user_count].password, 
                users[user_count].role, 
                users[user_count].status,
                users[user_count].full_name,
                users[user_count].birth_info,
                users[user_count].address,
                users[user_count].email,
                users[user_count].phone);
            
            // Jika baris database lama (hanya ada 5 data awal), isi default data kosong agar tidak crash
            if (parsed >= 5 && parsed < 10) {
                strcpy(users[user_count].full_name, "-");
                strcpy(users[user_count].birth_info, "-");
                strcpy(users[user_count].address, "-");
                strcpy(users[user_count].email, "-");
                strcpy(users[user_count].phone, "-");
            }
            user_count++;
        } fclose(f);
    }
    
    // Default Admin jika file kosong
    if (user_count == 0) {
        users[0].id = 1; 
        strcpy(users[0].username, "admin"); 
        strcpy(users[0].password, "admin123");
        strcpy(users[0].role, "ADMIN"); 
        strcpy(users[0].status, "ACTIVE"); 
        strcpy(users[0].full_name, "Administrator Resmi"); 
        strcpy(users[0].birth_info, "Jakarta, 01-01-1990");
        strcpy(users[0].address, "Kantor Pusat Blocky"); 
        strcpy(users[0].email, "admin@blocky.com");
        strcpy(users[0].phone, "08123456789");
        user_count++;
    }

    // --- Pemanggilan data produk dkk tetap sama seperti di bawah ini ---
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
        // Update header dengan informasi lengkap profil
        fprintf(f, "id,username,password,role,status,full_name,birth_info,address,email,phone\n");
        for (int i = 0; i < user_count; i++) {
            fprintf(f, "%d,%s,%s,%s,%s,%s,%s,%s,%s,%s\n", 
                users[i].id, 
                users[i].username, 
                users[i].password, 
                users[i].role, 
                users[i].status,
                users[i].full_name,
                users[i].birth_info,
                users[i].address,
                users[i].email,
                users[i].phone);
        }
        fclose(f);
    }
    
    // --- Bagian penyimpanan struct data lainnya tetap dipertahankan ---
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
        for (int i = 0; i < voucher_count; i++) fprintf(f, "%s,%d,%.2f,%.2f,%s\n", vouchers[voucher_count].voucher_code, vouchers[voucher_count].discount_percent, vouchers[voucher_count].max_discount_amount, vouchers[voucher_count].min_purchase, vouchers[voucher_count].status);
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
    printf("--- DETAIL PRODUK ---\n");
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
    clear_screen(); 
    int uid = users[logged_in_user_idx].id;
    print_warm_welcome_bar(users[logged_in_user_idx].username, "Keranjang Belanja");
    printf("\n%s[INFO]%s Kelola barang belanja Anda: ubah kuantitas, hapus item, atau checkout\n", COLOR_EMERALD, COLOR_RESET);
    printf("%s+--------+-----------------------------+----------+------------------+%s\n", COLOR_SEA_GREEN, COLOR_RESET);
    printf("%s| %-6s | %-27s | %-8s | %-16s |%s\n", COLOR_SEA_GREEN, "ID", "Nama Barang", "Jumlah", "Subtotal", COLOR_RESET);
    printf("%s+--------+-----------------------------+----------+------------------+%s\n", COLOR_SEA_GREEN, COLOR_RESET);

    double total = 0;
    int items_found = 0;
    int cart_item_ids[MAX_DATA];

    for (int i = 0; i < cart_count; i++) {
        if (carts[i].user_id == uid) {
            for (int j = 0; j < product_count; j++) {
                if (products[j].id == carts[i].product_id) {
                    double sub = products[j].price * carts[i].quantity;
                    total += sub;
                    printf("%s| %-6d | %-27.27s | %-8d | Rp%-14.2f |%s\n", COLOR_SEA_GREEN, products[j].id, products[j].name, carts[i].quantity, sub, COLOR_RESET);
                    cart_item_ids[items_found++] = products[j].id;
                }
            }
        }
    }

    if (items_found == 0) {
        printf("%s| %-62s |%s\n", COLOR_SEA_GREEN, "Keranjang Anda masih kosong", COLOR_RESET);
    }
    printf("%s+--------+-----------------------------+----------+------------------+%s\n", COLOR_SEA_GREEN, COLOR_RESET);
    printf("%s[TOTAL TAGIHAN]%s Rp%.2f\n", COLOR_LIME_GREEN, COLOR_RESET, total);

    printf("\n%s[PILIHAN AKSI]%s\n", COLOR_EMERALD, COLOR_RESET);
    printf("%s1.%s Ubah Kuantitas   %s2.%s Hapus Item   %s3.%s Checkout   %s4.%s Kembali\n", COLOR_LIME_GREEN, COLOR_RESET, COLOR_LIME_GREEN, COLOR_RESET, COLOR_LIME_GREEN, COLOR_RESET, COLOR_RED, COLOR_RESET);
    printf("Masukkan pilihan: ");
    int opt = read_int_safe();

    if (opt == 1) {
        if (items_found == 0) {
            printf("%s[!] Gagal: Tidak ada barang di dalam keranjang!\n%s", COLOR_RED, COLOR_RESET);
            press_enter_to_continue();
            return;
        }
        printf("Masukkan ID Produk: ");
        int pid = read_int_safe();
        bool in_cart = false;
        for (int i = 0; i < items_found; i++) {
            if (cart_item_ids[i] == pid) in_cart = true;
        }
        if (!in_cart) {
            printf("%s[!] Produk tidak ada di keranjang!\n%s", COLOR_RED, COLOR_RESET);
            press_enter_to_continue();
            return;
        }
        printf("Masukkan Kuantitas Baru: ");
        int nqty = read_int_safe();
        if (nqty <= 0) {
            printf("%s[!] Kuantitas harus lebih dari 0. Gunakan menu Hapus jika ingin menghapus.\n%s", COLOR_RED, COLOR_RESET);
        } else {
            for (int i = 0; i < cart_count; i++) {
                if (carts[i].user_id == uid && carts[i].product_id == pid) {
                    carts[i].quantity = nqty;
                    break;
                }
            }
            save_data();
            printf("%s[OK] Kuantitas berhasil diperbarui!\n%s", COLOR_PINE_GREEN, COLOR_RESET);
        }
        press_enter_to_continue();
    } 
    else if (opt == 2) {
        if (items_found == 0) {
            printf("%s[!] Gagal: Tidak ada barang di dalam keranjang!\n%s", COLOR_RED, COLOR_RESET);
            press_enter_to_continue();
            return;
        }
        printf("Masukkan ID Produk: ");
        int pid = read_int_safe();
        bool in_cart = false;
        for (int i = 0; i < items_found; i++) {
            if (cart_item_ids[i] == pid) in_cart = true;
        }
        if (!in_cart) {
            printf("%s[!] Produk tidak ada di keranjang!\n%s", COLOR_RED, COLOR_RESET);
            press_enter_to_continue();
            return;
        }
        for (int i = 0; i < cart_count; i++) {
            if (carts[i].user_id == uid && carts[i].product_id == pid) {
                for (int j = i; j < cart_count - 1; j++) carts[j] = carts[j + 1];
                cart_count--;
                break;
            }
        }
        save_data();
        printf("%s[OK] Item dihapus dari keranjang.\n%s", COLOR_PINE_GREEN, COLOR_RESET);
        press_enter_to_continue();
    } 
    else if (opt == 3) {
        if (total <= 0) {
            printf("%s[!] Keranjang masih kosong!\n%s", COLOR_RED, COLOR_RESET);
            press_enter_to_continue();
            return;
        }
        printf("Kode Voucher (Kosongkan jika tidak ada): ");
        char vcode[MAX_STR];
        read_string_safe(vcode, MAX_STR);
        double disc = 0;
        if (strlen(vcode) > 0) {
            bool v_found = false;
            for (int i = 0; i < voucher_count; i++) {
                if (strcmp(vouchers[i].voucher_code, vcode) == 0 && strcmp(vouchers[i].status, "ACTIVE") == 0) {
                    v_found = true;
                    if (total >= vouchers[i].min_purchase) {
                        disc = (vouchers[i].discount_percent / 100.0) * total;
                        if (disc > vouchers[i].max_discount_amount) disc = vouchers[i].max_discount_amount;
                        printf("%s[OK] Voucher berhasil dipasang! Potongan: Rp%.2f\n%s", COLOR_PINE_GREEN, disc, COLOR_RESET);
                    } else {
                        printf("%s[!] Total belanja belum memenuhi minimum pembelian voucher!\n%s", COLOR_RED, COLOR_RESET);
                    }
                    break;
                }
            }
            if (!v_found) printf("%s[!] Kode voucher tidak valid atau sudah tidak aktif!\n%s", COLOR_RED, COLOR_RESET);
        }
        
        double final_total = total - disc;
        if (final_total < 0) final_total = 0;
        printf("Total Pembayaran: Rp%.2f\n", final_total);
        printf("Pilih Metode Pembayaran (COD/TRANSFER/EWALLET): ");
        char pay_method[MAX_STR];
        read_string_safe(pay_method, MAX_STR);
        printf("Pilih Metode Pengiriman (REGULER/EKSPRES): ");
        char ship_method[MAX_STR];
        read_string_safe(ship_method, MAX_STR);

        int new_order_id = (order_count > 0) ? orders[order_count - 1].order_id + 1 : 1;
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);
        char date_str[MAX_STR];
        sprintf(date_str, "%02d-%02d-%04d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);

        int seller_id = 1; 
        Order o;
        o.order_id = new_order_id;
        o.customer_id = uid;
        o.seller_id = seller_id;
        o.total_amount = final_total;
        strcpy(o.voucher_code, strlen(vcode) > 0 ? vcode : "NONE");
        strcpy(o.payment_method, pay_method);
        strcpy(o.shipping_method, ship_method);
        strcpy(o.status, "PENDING");
        strcpy(o.order_date, date_str);

        if (order_count < MAX_DATA) {
            orders[order_count++] = o;
        }

        for (int i = 0; i < cart_count; i++) {
            if (carts[i].user_id == uid) {
                for (int j = 0; j < product_count; j++) {
                    if (products[j].id == carts[i].product_id) {
                        OrderItem oi;
                        oi.order_id = new_order_id;
                        oi.product_id = products[j].id;
                        oi.quantity = carts[i].quantity;
                        oi.price_at_purchase = products[j].price;
                        if (order_item_count < MAX_DATA) {
                            order_items[order_item_count++] = oi;
                        }
                        products[j].stock -= carts[i].quantity;
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
        save_data();
        printf("%s[OK] Pesanan berhasil diproses dengan ID %d!\n%s", COLOR_PINE_GREEN, new_order_id, COLOR_RESET);
        press_enter_to_continue();
    }
}

void add_to_recent_view(int pid) {
    for (int i = 0; i < recent_count; i++) {
        if (recents[i].product_id == pid) return;
    }
    if (recent_count < MAX_DATA) recents[recent_count++].product_id = pid;
}

void manage_wishlist_menu() {
    clear_screen();
    int uid = users[logged_in_user_idx].id;
    print_warm_welcome_bar(users[logged_in_user_idx].username, "Wishlist Favorit");
    printf("\n%s[INFO]%s Produk-produk pilihan Anda. Pindahkan ke keranjang untuk membeli\n", COLOR_EMERALD, COLOR_RESET);
    printf("%s+--------+-----------------------------+------------------+\n%s", COLOR_SEA_GREEN, COLOR_RESET);
    printf("%s| %-6s | %-27s | %-16s |\n%s", COLOR_SEA_GREEN, "ID", "Nama Barang", "Harga", COLOR_RESET);
    printf("%s+--------+-----------------------------+------------------+\n%s", COLOR_SEA_GREEN, COLOR_RESET);

    int items_found = 0;
    int wishlist_item_ids[MAX_DATA];

    for (int i = 0; i < wishlist_count; i++) {
        if (wishlists[i].user_id == uid) {
            for (int j = 0; j < product_count; j++) {
                if (products[j].id == wishlists[i].product_id) {
                    printf("%s| %-6d | %-27.27s | Rp%-14.2f |\n%s", COLOR_SEA_GREEN, products[j].id, products[j].name, products[j].price, COLOR_RESET);
                    wishlist_item_ids[items_found++] = products[j].id;
                }
            }
        }
    }

    if (items_found == 0) {
        printf("%s| %-60s |\n%s", COLOR_SEA_GREEN, "Wishlist Anda masih kosong", COLOR_RESET);
    }
    printf("%s+--------+-----------------------------+------------------+\n%s", COLOR_SEA_GREEN, COLOR_RESET);

    printf("\n%s[PILIHAN AKSI]%s\n", COLOR_EMERALD, COLOR_RESET);
    printf("%s1.%s Pindah ke Keranjang   %s2.%s Kembali\n", COLOR_LIME_GREEN, COLOR_RESET, COLOR_RED, COLOR_RESET);
    printf("Masukkan pilihan: ");
    int opt = read_int_safe();

    if (opt == 1) {
        if (items_found == 0) {
            printf("%s[!] Gagal: Tidak ada barang di dalam Wishlist untuk dipindahkan!\n%s", COLOR_RED, COLOR_RESET);
            press_enter_to_continue();
            return;
        }
        printf("Masukkan ID Produk: ");
        int pid = read_int_safe();
        bool found_in_list = false;
        for (int i = 0; i < items_found; i++) {
            if (wishlist_item_ids[i] == pid) found_in_list = true;
        }
        if (!found_in_list) {
            printf("%s[!] Produk tidak ada di wishlist Anda!\n%s", COLOR_RED, COLOR_RESET);
            press_enter_to_continue();
            return;
        }
        
        add_to_cart(pid, 1);
        
        for (int i = 0; i < wishlist_count; i++) {
            if (wishlists[i].user_id == uid && wishlists[i].product_id == pid) {
                for (int j = i; j < wishlist_count - 1; j++) wishlists[j] = wishlists[j + 1];
                wishlist_count--;
                break;
            }
        }
        save_data();
        printf("%s[OK] Produk dipindahkan ke keranjang.\n%s", COLOR_PINE_GREEN, COLOR_RESET);
        press_enter_to_continue();
    }
}

void view_customer_orders() {
    clear_screen();
    int uid = users[logged_in_user_idx].id;
    print_warm_welcome_bar(users[logged_in_user_idx].username, "Riwayat Pesanan Pelanggan");
    printf("\n%s[INFO]%s Kelola pesanan Anda: konfirmasi, ulasan, pembatalan, atau retur\n", COLOR_EMERALD, COLOR_RESET);
    printf("%s+--------+-----------------+------------+------------+\n%s", COLOR_SEA_GREEN, COLOR_RESET);
    printf("%s| %-6s | %-15s | %-10s | %-10s |\n%s", COLOR_SEA_GREEN, "Order", "Total Harga", "Status", "Tanggal", COLOR_RESET);
    printf("%s+--------+-----------------+------------+------------+\n%s", COLOR_SEA_GREEN, COLOR_RESET);

    int items_found = 0;
    for (int i = 0; i < order_count; i++) {
        if (orders[i].customer_id == uid) {
            printf("%s| %-6d | Rp%-13.2f | %-10.10s | %-10.10s |\n%s", COLOR_SEA_GREEN, orders[i].order_id, orders[i].total_amount, orders[i].status, orders[i].order_date, COLOR_RESET);
            items_found++;
        }
    }

    if (items_found == 0) {
        printf("%s| %-46s |\n%s", COLOR_SEA_GREEN, "Belum ada riwayat pesanan", COLOR_RESET);
    }
    printf("%s+--------+-----------------+------------+------------+\n%s", COLOR_SEA_GREEN, COLOR_RESET);

    printf("\n%s[PILIHAN AKSI]%s\n", COLOR_EMERALD, COLOR_RESET);
    printf("%s1.%s Konfirmasi Penyelesaian   %s2.%s Beri Ulasan   %s3.%s Ajukan Retur\n", COLOR_LIME_GREEN, COLOR_RESET, COLOR_LIME_GREEN, COLOR_RESET, COLOR_LIME_GREEN, COLOR_RESET);
    printf("%s4.%s Batalkan Pembelian        %s5.%s Detail Pesanan  %s6.%s Kembali\n", COLOR_RED, COLOR_RESET, COLOR_LIME_GREEN, COLOR_RESET, COLOR_RED, COLOR_RESET);
    printf("Masukkan pilihan: ");
    int opt = read_int_safe();

    if (opt == 1) {
        printf("Masukkan ID Order: ");
        int oid = read_int_safe();
        bool found = false;
        for (int i = 0; i < order_count; i++) {
            if (orders[i].order_id == oid && orders[i].customer_id == uid) {
                strcpy(orders[i].status, "COMPLETED");
                found = true;
                printf("%s[OK] Pesanan #%d telah diselesaikan. Terima kasih!\n%s", COLOR_PINE_GREEN, oid, COLOR_RESET);
                break;
            }
        }
        if (!found) printf("%s[!] Order ID tidak ditemukan!\n%s", COLOR_RED, COLOR_RESET);
        save_data();
        press_enter_to_continue();
    } 
    else if (opt == 2) {
        printf("Masukkan ID Order: ");
        int oid = read_int_safe();
        bool order_exists = false;
        for (int i = 0; i < order_count; i++) {
            if (orders[i].order_id == oid && orders[i].customer_id == uid) order_exists = true;
        }
        if (!order_exists) {
            printf("%s[!] Order ID tidak ditemukan!\n%s", COLOR_RED, COLOR_RESET);
            press_enter_to_continue();
            return;
        }
        
        printf("Masukkan ID Produk dari order ini yang ingin diulas: ");
        int pid = read_int_safe();
        printf("Masukkan Rating (1-5): ");
        int rate = read_int_safe();
        if (rate < 1 || rate > 5) {
            printf("%s[!] Rating tidak valid!\n%s", COLOR_RED, COLOR_RESET);
            press_enter_to_continue();
            return;
        }
        printf("Masukkan Komentar/Ulasan: ");
        char comm[MAX_STR];
        read_string_safe(comm, MAX_STR);

        int nid = (review_count > 0) ? reviews[review_count - 1].review_id + 1 : 1;
        Review r;
        r.review_id = nid;
        r.order_id = oid;
        r.product_id = pid;
        r.customer_id = uid;
        r.rating = rate;
        strcpy(r.comment, comm);

        if (review_count < MAX_DATA) {
            reviews[review_count++] = r;
            save_data();
            printf("%s[OK] Ulasan Anda berhasil disimpan!\n%s", COLOR_PINE_GREEN, COLOR_RESET);
        }
        press_enter_to_continue();
    } 
    else if (opt == 3) {
        printf("Masukkan ID Order: ");
        int oid = read_int_safe();
        for (int i = 0; i < order_count; i++) {
            if (orders[i].order_id == oid && orders[i].customer_id == uid) {
                if (strcmp(orders[i].status, "COMPLETED") == 0) {
                    printf("%s[!] Gagal: Barang sudah berstatus Selesai, tidak bisa retur.\n%s", COLOR_RED, COLOR_RESET);
                } else {
                    strcpy(orders[i].status, "RETURNED");
                    printf("%s[OK] Pengajuan retur untuk pesanan #%d berhasil diproses.\n%s", COLOR_PINE_GREEN, oid, COLOR_RESET);
                    save_data();
                }
                break;
            }
        }
        press_enter_to_continue();
    } 
    else if (opt == 4) {
        printf("Masukkan ID Order yang ingin dibatalkan: ");
        int oid = read_int_safe();
        for (int i = 0; i < order_count; i++) {
            if (orders[i].order_id == oid && orders[i].customer_id == uid) {
                if (strcmp(orders[i].status, "SHIPPED") == 0 || strcmp(orders[i].status, "COMPLETED") == 0) {
                    printf("%s[!] Gagal: Pesanan sudah dalam pengiriman atau sudah selesai.\n%s", COLOR_RED, COLOR_RESET);
                } else {
                    printf("%sApakah Anda yakin ingin membatalkan pesanan #%d? (Y/N): %s", COLOR_RED, oid, COLOR_RESET);
                    char confirm[10];
                    read_string_safe(confirm, 10);
                    if (confirm[0] == 'Y' || confirm[0] == 'y') {
                        strcpy(orders[i].status, "CANCELLED");
                        printf("%s[OK] Pesanan #%d berhasil dibatalkan.\n%s", COLOR_PINE_GREEN, oid, COLOR_RESET);
                        save_data();
                    } else {
                        printf("Pembatalan dibatalkan.\n");
                    }
                }
                break;
            }
        }
        press_enter_to_continue();
    } 
    else if (opt == 5) {
        printf("Masukkan ID Order: ");
        int oid = read_int_safe();
        printf("\n--- DETAIL PESANAN #%d ---\n", oid);
        printf("%s+------------+------------+------------------+\n%s", COLOR_SEA_GREEN, COLOR_RESET);
        printf("%s| Product ID | Kuantitas  | Harga Satuan     |\n%s", COLOR_SEA_GREEN, COLOR_RESET);
        printf("%s+------------+------------+------------------+\n%s", COLOR_SEA_GREEN, COLOR_RESET);
        for (int i = 0; i < order_item_count; i++) {
            if (order_items[i].order_id == oid) {
                printf("%s| %-10d | %-10d | Rp%-14.2f |\n%s", COLOR_SEA_GREEN, order_items[i].product_id, order_items[i].quantity, order_items[i].price_at_purchase, COLOR_RESET);
            }
        }
        printf("%s+------------+------------+------------------+\n%s", COLOR_SEA_GREEN, COLOR_RESET);
        press_enter_to_continue();
    }
}

void process_direct_order(int product_id, int quantity) {
    int p_idx = product_id - 1; // Asumsi ID produk berurutan dengan indeks (ID 1 = indeks 0)
    
    // Validasi dasar keamanan stok dan ID produk
    if (p_idx < 0 || p_idx >= product_count || quantity <= 0) {
        printf("%s[!] Gagal: Produk tidak valid.%s\n", COLOR_RED, COLOR_RESET);
        return;
    }
    if (products[p_idx].stock < quantity) {
        printf("%s[!] Gagal: Stok tidak mencukupi (Sisa stok: %d).%s\n", COLOR_RED, products[p_idx].stock, COLOR_RESET);
        return;
    }

    double total_price = products[p_idx].price * quantity;

    // Tampilkan Ringkasan Pembelian / Nota Checkout
    clear_screen();
    printf("%s--- RINGKASAN PEMBELIAN (CHECKOUT) ---%s\n\n", COLOR_B_WHITE, COLOR_RESET);
    printf("Nama Barang   : %s\n", products[p_idx].name);
    printf("Jumlah Pesan  : %d Unit\n", quantity);
    printf("Total Tagihan : Rp%.2f\n", total_price);

    // MANAJEMEN PENGIRIMAN: Mengambil alamat utama user & opsi ubah alamat temporer
    char current_shipping_address[MAX_STR];
    strcpy(current_shipping_address, users[logged_in_user_idx].address); // Set default dari database profile

    printf("\n%s[KONFIRMASI ALAMAT PENGIRIMAN]%s\n", COLOR_EMERALD, COLOR_RESET);
    printf("Alamat Kirim Utama Anda   : %s%s%s\n", COLOR_LIME_GREEN, current_shipping_address, COLOR_RESET);
    printf("Apakah ingin memakai alamat baru khusus untuk pesanan ini? (y/n): ");
    char change_addr[10];
    read_string_safe(change_addr, 10);

    if (change_addr[0] == 'y' || change_addr[0] == 'Y') {
        printf("Masukkan Alamat Pengiriman Baru: ");
        read_string_safe(current_shipping_address, MAX_STR);
        printf("%s[OK] Alamat tujuan berhasil dialihkan ke alamat alternatif.%s\n", COLOR_PINE_GREEN, COLOR_RESET);
    }

    printf("\nKonfirmasi penyelesaian order sekarang? (y/n): ");
    char confirm[10];
    read_string_safe(confirm, 10);

    if (confirm[0] == 'y' || confirm[0] == 'Y') {
        // 1. Injeksi data transaksi baru ke struk order utama (orders.csv)
        int new_order_id = order_count > 0 ? orders[order_count - 1].order_id + 1 : 1;
        orders[order_count].order_id = new_order_id;
        orders[order_count].customer_id = users[logged_in_user_idx].id;
        orders[order_count].seller_id = products[p_idx].seller_id;
        orders[order_count].total_amount = total_price;
        
        strcpy(orders[order_count].voucher_code, "-");
        strcpy(orders[order_count].payment_method, "Transfer Instan");
        strcpy(orders[order_count].shipping_method, "Reguler");
        strcpy(orders[order_count].status, "PENDING");
        
        // Mengambil timestamp sistem lokal untuk tanggal order
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);
        sprintf(orders[order_count].order_date, "%02d-%02d-%04d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
        order_count++;

        // 2. Injeksi ke rincian item order detail (order_items.csv)
        order_items[order_item_count].order_id = new_order_id;
        order_items[order_item_count].product_id = products[p_idx].id;
        order_items[order_item_count].quantity = quantity;
        order_items[order_item_count].price_at_purchase = products[p_idx].price;
        order_item_count++;

        // 3. Pengurangan kuota stok gudang & penambahan akumulasi produk terlaris
        products[p_idx].stock -= quantity;
        products[p_idx].sales_count += quantity;

        save_data(); // Amankan data baru langsung ke database CSV file
        
        printf("\n%s[SUKSES] Pembelian berhasil diproses!%s\n", COLOR_PINE_GREEN, COLOR_RESET);
        printf("Paket Anda akan dikirim ke alamat: %s%s%s\n", COLOR_LIME_GREEN, current_shipping_address, COLOR_RESET);
    } else {
        printf("\n%s[!] Pembelian dibatalkan.%s\n", COLOR_RED, COLOR_RESET);
    }
    press_enter_to_continue();
}

void execution_search_menu() {
    clear_screen();
    print_warm_welcome_bar(users[logged_in_user_idx].username, "Pencarian Produk");
    printf("\n%s[INFO]%s Temukan produk yang Anda cari dengan berbagai kriteria\n\n", COLOR_EMERALD, COLOR_RESET);
    printf("%s[Pencarian Terakhir]%s %s\n", COLOR_EMERALD, COLOR_RESET, last_search_keyword);
    printf("%s[Trending Sekarang]%s ", COLOR_LIME_GREEN, COLOR_RESET);
    int top_search_idx = -1; int max_search = -1;
    for (int i = 0; i < product_count; i++) {
        if (products[i].search_count > max_search) { max_search = products[i].search_count; top_search_idx = i; }
    }
    if(top_search_idx != -1) printf("%s%s (%dx pencarian)%s\n", COLOR_LIME_GREEN, products[top_search_idx].name, products[top_search_idx].search_count, COLOR_RESET);
    else printf("%s[Tidak ada data trending]%s\n", COLOR_RED, COLOR_RESET);
    
    int type;
    do {
        printf("\n%s[KRITERIA PENCARIAN]%s\n", COLOR_EMERALD, COLOR_RESET);
        printf("%s1.%s Nama Produk   %s2.%s Merek/Brand   %s3.%s Kategori   %s4.%s Lihat Semua\n", COLOR_LIME_GREEN, COLOR_RESET, COLOR_LIME_GREEN, COLOR_RESET, COLOR_LIME_GREEN, COLOR_RESET, COLOR_LIME_GREEN, COLOR_RESET);
        printf("\nPilihan: ");
        type = read_int_safe();
        if (type < 1 || type > 4) printf("%s[!] Pilihan tidak valid. Silakan masukkan angka 1 hingga 4.%s\n", COLOR_RED, COLOR_RESET);
    } while (type < 1 || type > 4);
    
    int matched_ids[MAX_DATA]; int total = 0;
    if (type >= 1 && type <= 3) {
        if (type == 3) {
            printf("\n%s[Kategori Tersedia]%s\n", COLOR_EMERALD, COLOR_RESET);
            char seen[MAX_DATA][MAX_STR];
            int seen_count = 0;
            for (int i = 0; i < product_count; i++) {
                bool found = false;
                for (int j = 0; j < seen_count; j++) {
                    if (strcmp(seen[j], products[i].category) == 0) { found = true; break; }
                }
                if (!found) {
                    strcpy(seen[seen_count++], products[i].category);
                    printf("%s- %s%s\n", COLOR_LIME_GREEN, products[i].category, COLOR_RESET);
                }
            }
            printf("\n");
        }

        printf("Masukkan Kata Kunci: "); char keyword[MAX_STR]; read_string_safe(keyword, MAX_STR);
        strcpy(last_search_keyword, keyword);
        TrieNode *target_root = (type == 1) ? root_product_name : ((type == 2) ? root_brand : root_category);
        total = search_trie_prefix(target_root, keyword, matched_ids);
    } else {
        total = product_count;
        for (int i = 0; i < product_count; i++) matched_ids[i] = products[i].id;
    }
    
    if (total == 0) { printf("\n%s[HASIL PENCARIAN]%s Produk tidak ditemukan%s\n", COLOR_RED, COLOR_RESET, COLOR_RESET); press_enter_to_continue(); return; }
    
    printf("\n%s[PENGURUTAN]%s\n", COLOR_EMERALD, COLOR_RESET);
    printf("%s0.%s Tidak diurutkan   %s1.%s Harga Terendah   %s2.%s Terlaris\n", COLOR_LIME_GREEN, COLOR_RESET, COLOR_LIME_GREEN, COLOR_RESET, COLOR_LIME_GREEN, COLOR_RESET);
    printf("\nPilihan: ");
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

    clear_screen();
    printf("%s[HASIL PENCARIAN - %d PRODUK DITEMUKAN]%s\n", COLOR_EMERALD, total, COLOR_RESET);
    printf("%s+-----+---------------------------+----------+----------------+------+%s\n", COLOR_SEA_GREEN, COLOR_RESET);
    printf("%s| %-3s | %-25s | %-8s | %-14s | %-4s |%s\n", COLOR_SEA_GREEN, "ID", "Nama Produk", "Brand", "Harga", "Stok", COLOR_RESET);
    printf("%s+-----+---------------------------+----------+----------------+------+%s\n", COLOR_SEA_GREEN, COLOR_RESET);
    for (int i = 0; i < total; i++) {
        int idx = matched_ids[i] - 1; products[idx].search_count++;
        printf("%s| %-3d | %-25.25s | %-8.8s | Rp%-12.2f | %-4d |%s\n", COLOR_SEA_GREEN, products[idx].id, products[idx].name, products[idx].brand, products[idx].price, products[idx].stock, COLOR_RESET);
    }
    printf("%s+-----+---------------------------+----------+----------------+------+%s\n", COLOR_SEA_GREEN, COLOR_RESET);
    
    if (total > 0) {
        bool stay_in_search = true;
        while (stay_in_search) {
            printf("\n%s[AKSI PENCARIAN]%s\n", COLOR_EMERALD, COLOR_RESET);
            printf("%s1.%s Beli Sekarang   %s2.%s Tambah Keranjang  %s3.%s Tambah Wishlist   %s4.%s Detail Produk   %s5.%s Batal\n", 
                COLOR_LIME_GREEN, COLOR_RESET, COLOR_LIME_GREEN, COLOR_RESET, COLOR_LIME_GREEN, COLOR_RESET, COLOR_LIME_GREEN, COLOR_RESET, COLOR_RED, COLOR_RESET);
            printf("Pilih aksi: ");
            int action = read_int_safe();

            switch (action) {
                case 1: { 
                    printf("\nMasukkan ID Produk yang ingin dibeli (0 untuk batal): ");
                    int target_id = read_int_safe();
                    if (target_id == 0) {
                        printf("%s[INFO] Pembelian dibatalkan.%s\n", COLOR_WHITE, COLOR_RESET);
                        press_enter_to_continue();
                        break; 
                    }

                    printf("Masukkan Jumlah: ");
                    int qty = read_int_safe();
                    if (qty <= 0) {
                        printf("%s[!] Jumlah tidak valid.%s\n", COLOR_RED, COLOR_RESET);
                        press_enter_to_continue();
                        break;
                    }
                    
                    process_direct_order(target_id, qty);
                    stay_in_search = false; // Keluar dari loop setelah transaksi selesai
                    break; 
                }
                case 2: {
                    printf("\nMasukkan ID Produk yang ingin ditambah ke keranjang (0 untuk batal): ");
                    int target_id = read_int_safe();
                    if (target_id == 0) {
                        printf("%s[INFO] Aksi dibatalkan.%s\n", COLOR_WHITE, COLOR_RESET);
                        press_enter_to_continue();
                        break;
                    }
                    
                    bool valid_product = false;
                    int p_idx = -1;
                    for (int i = 0; i < total; i++) {
                        if (matched_ids[i] == target_id) {
                            valid_product = true;
                            p_idx = target_id - 1;
                            break;
                        }
                    }

                    if (!valid_product) {
                        printf("%s[!] ID Produk tidak ditemukan dalam hasil pencarian.%s\n", COLOR_RED, COLOR_RESET);
                        press_enter_to_continue();
                    } else {
                        printf("Masukkan Jumlah (0 untuk batal): ");
                        int qty = read_int_safe();
                        if (qty == 0) {
                            printf("%s[INFO] Aksi dibatalkan.%s\n", COLOR_WHITE, COLOR_RESET);
                            press_enter_to_continue();
                            break;
                        }

                        if (qty <= 0 || qty > products[p_idx].stock) {
                            printf("%s[!] Stok tidak mencukupi atau jumlah tidak valid.%s\n", COLOR_RED, COLOR_RESET);
                            press_enter_to_continue();
                        } else {
                            bool in_cart = false;
                            for (int i = 0; i < cart_count; i++) {
                                if (carts[i].user_id == users[logged_in_user_idx].id && carts[i].product_id == target_id) {
                                    carts[i].quantity += qty;
                                    in_cart = true;
                                    break;
                                }
                            }
                            if (!in_cart && cart_count < MAX_DATA) {
                                carts[cart_count].user_id = users[logged_in_user_idx].id;
                                carts[cart_count].product_id = target_id;
                                carts[cart_count].quantity = qty;
                                cart_count++;
                            }
                            save_data();
                            printf("%s[OK] Berhasil ditambahkan ke keranjang!%s\n", COLOR_PINE_GREEN, COLOR_RESET);
                            press_enter_to_continue();
                            stay_in_search = false; // Keluar dari loop setelah berhasil menambah ke keranjang
                        }
                    }
                    break;
                }
                case 3: {
                    printf("Masukkan ID Produk untuk masuk ke Wishlist (0 untuk batal): "); 
                    int pid = read_int_safe();
                    if (pid == 0) {
                        printf("%s[INFO] Aksi dibatalkan.%s\n", COLOR_WHITE, COLOR_RESET);
                        press_enter_to_continue();
                        break;
                    }

                    if (wishlist_count < MAX_DATA) {
                        wishlists[wishlist_count].user_id = users[logged_in_user_idx].id; 
                        wishlists[wishlist_count].product_id = pid;
                        wishlist_count++; 
                        save_data(); 
                        printf("%s[OK] Disimpan ke wishlist!%s\n", COLOR_PINE_GREEN, COLOR_RESET);
                        press_enter_to_continue();
                        stay_in_search = false; // Keluar dari loop setelah sukses masuk wishlist
                    }
                    break;
                }
                case 4: {
                    printf("Masukkan ID Produk (0 untuk batal): "); 
                    int pid = read_int_safe();
                    if (pid == 0) {
                        printf("%s[INFO] Aksi dibatalkan.%s\n", COLOR_WHITE, COLOR_RESET);
                        press_enter_to_continue();
                        break;
                    }

                    add_to_recent_view(pid); 
                    clear_screen(); 
                    display_product_details(pid - 1);
                    stay_in_search = false; // Keluar dari loop setelah melihat detail produk
                    break;
                }
                case 5: {
                    stay_in_search = false;
                    break;
                }
                default: {
                    printf("%s[!] Pilihan tidak valid.%s\n", COLOR_RED, COLOR_RESET);
                    press_enter_to_continue();
                    break;
                }
            }
        }
    } else {
        printf("\n%sTekan ENTER untuk kembali...%s\n", COLOR_WHITE, COLOR_RESET);
        press_enter_to_continue();
    }
}

void edit_profile_menu() {
    bool viewing_profile = true;
    bool mask_password = true; 

    // Pengaman jika fungsi dipanggil saat tidak ada pengguna yang login
    if (logged_in_user_idx < 0 || logged_in_user_idx >= user_count) {
        printf("%s[!] Error: Sesi login tidak valid.%s\n", COLOR_RED, COLOR_RESET);
        press_enter_to_continue();
        return;
    }

    while (viewing_profile) {
        clear_screen();
        int idx = logged_in_user_idx; // Menggunakan indeks user aktif saat ini
        print_warm_welcome_bar(users[idx].username, "Informasi Profil Pengguna");

        printf("\n%s[DATA PROFIL ANDA]%s\n", COLOR_EMERALD, COLOR_RESET);
        printf("%s+---------------------+------------------------------------------+%s\n", COLOR_SEA_GREEN, COLOR_RESET);
        printf("%s| %-19s | %-40s |%s\n", COLOR_SEA_GREEN, "Kategori", "Detail Informasi", COLOR_RESET);
        printf("%s+---------------------+------------------------------------------+%s\n", COLOR_SEA_GREEN, COLOR_RESET);
        
        printf("%s| %-19s | %-40d |%s\n", COLOR_SEA_GREEN, "ID Pengguna", users[idx].id, COLOR_RESET);
        printf("%s| %-19s | %-40s |%s\n", COLOR_SEA_GREEN, "Username", users[idx].username, COLOR_RESET);
        
        if (mask_password) {
            printf("%s| %-19s | %-40s |%s\n", COLOR_SEA_GREEN, "Password", "****** (Terproteksi - Pilih Opsi 2)", COLOR_RESET);
        } else {
            printf("%s| %-19s | %-40s |%s\n", COLOR_SEA_GREEN, "Password", users[idx].password, COLOR_RESET);
        }

        printf("%s| %-19s | %-40s |%s\n", COLOR_SEA_GREEN, "Nama Lengkap", users[idx].full_name, COLOR_RESET);
        printf("%s| %-19s | %-40s |%s\n", COLOR_SEA_GREEN, "Tempat, Tgl Lahir", users[idx].birth_info, COLOR_RESET);
        printf("%s| %-19s | %-40s |%s\n", COLOR_SEA_GREEN, "Alamat Pengiriman", users[idx].address, COLOR_RESET);
        printf("%s| %-19s | %-40s |%s\n", COLOR_SEA_GREEN, "Alamat Email", users[idx].email, COLOR_RESET);
        printf("%s| %-19s | %-40s |%s\n", COLOR_SEA_GREEN, "Nomor Telepon", users[idx].phone, COLOR_RESET);
        printf("%s+---------------------+------------------------------------------+%s\n", COLOR_SEA_GREEN, COLOR_RESET);

        printf("\n%s[PILIHAN AKSI]%s\n", COLOR_EMERALD, COLOR_RESET);
        printf("%s1.%s Ubah Data Profil\n", COLOR_LIME_GREEN, COLOR_RESET);
        if (mask_password) {
            printf("%s2.%s Lihat Teks Password\n", COLOR_LIME_GREEN, COLOR_RESET);
        } else {
            printf("%s2.%s Sembunyikan Teks Password\n", COLOR_LIME_GREEN, COLOR_RESET);
        }
        printf("%s3.%s Kembali\n", COLOR_RED, COLOR_RESET);
        
        printf("\nMasukkan pilihan: ");
        int opt = read_int_safe();

        if (opt == 1) {
            printf("\n--- %sFormulir Pembaruan Profil%s ---\n", COLOR_B_WHITE, COLOR_RESET);
            printf("%s(Tekan ENTER langsung tanpa mengetik untuk mempertahankan data lama)%s\n\n", COLOR_WHITE, COLOR_RESET);
            
            printf("Username Baru [%s]: ", users[idx].username);
            char temp_username[MAX_STR];
            read_string_safe(temp_username, MAX_STR);
            if (strlen(temp_username) > 0 && strcmp(temp_username, users[idx].username) != 0) {
                bool dup = false;
                for (int i = 0; i < user_count; i++) {
                    if (strcmp(users[i].username, temp_username) == 0) { dup = true; break; }
                }
                if (dup) {
                    printf("%s[!] Gagal: Username tersebut sudah terpakai!%s\n", COLOR_RED, COLOR_RESET);
                    press_enter_to_continue();
                    continue;
                }
                strcpy(users[idx].username, temp_username);
            }

            printf("Password Baru [%s]: ", mask_password ? "******" : users[idx].password);
            char temp_pass[MAX_STR];
            read_string_safe(temp_pass, MAX_STR);
            if (strlen(temp_pass) > 0) strcpy(users[idx].password, temp_pass);

            printf("Nama Lengkap Baru [%s]: ", users[idx].full_name);
            char temp_name[MAX_STR];
            read_string_safe(temp_name, MAX_STR);
            if (strlen(temp_name) > 0) strcpy(users[idx].full_name, temp_name);

            printf("Tempat, Tgl Lahir Baru [%s]: ", users[idx].birth_info);
            char temp_birth[MAX_STR];
            read_string_safe(temp_birth, MAX_STR);
            if (strlen(temp_birth) > 0) strcpy(users[idx].birth_info, temp_birth);

            printf("Alamat Pengiriman Baru [%s]: ", users[idx].address);
            char temp_address[MAX_STR];
            read_string_safe(temp_address, MAX_STR);
            if (strlen(temp_address) > 0) strcpy(users[idx].address, temp_address);

            printf("Alamat Email Baru [%s]: ", users[idx].email);
            char temp_email[MAX_STR];
            read_string_safe(temp_email, MAX_STR);
            if (strlen(temp_email) > 0) strcpy(users[idx].email, temp_email);

            printf("Nomor Telepon Baru [%s]: ", users[idx].phone);
            char temp_phone[MAX_STR];
            read_string_safe(temp_phone, MAX_STR);
            if (strlen(temp_phone) > 0) strcpy(users[idx].phone, temp_phone);

            save_data(); // Simpan perubahan ke file CSV secara permanen
            printf("\n%s[OK] Profil Berhasil Diperbarui!%s\n", COLOR_PINE_GREEN, COLOR_RESET);
            press_enter_to_continue();
        } 
        else if (opt == 2) {
            mask_password = !mask_password;
        } 
        else if (opt == 3) {
            viewing_profile = false; 
        }
    }
}

void customer_menu() {
    int choice;
    do {
        clear_screen(); print_warm_welcome_bar(users[logged_in_user_idx].username, "Dasbor Pelanggan");
        
        printf("\n%s[AKTIVITAS TERAKHIR]%s\n", COLOR_EMERALD, COLOR_RESET);
        printf("%s+------------------------------------------------------------------+%s\n", COLOR_SEA_GREEN, COLOR_RESET);
        if(recent_count == 0) {
            printf("%s| %-64s |%s\n", COLOR_SEA_GREEN, "Belum ada produk yang dilihat", COLOR_RESET);
        } else {
            for(int i=0; i<recent_count && i<5; i++) {
                int p_idx = recents[i].product_id - 1;
                printf("%s| ID: %-4d | %-50.50s |%s\n", COLOR_SEA_GREEN, products[p_idx].id, products[p_idx].name, COLOR_RESET);
            }
            if(recent_count > 5) printf("%s| ... dan %d produk lainnya                                  |%s\n", COLOR_SEA_GREEN, recent_count - 5, COLOR_RESET);
        }
        printf("%s+------------------------------------------------------------------+%s\n", COLOR_SEA_GREEN, COLOR_RESET);
        
        printf("\n%s[PRODUK UNGGULAN]%s\n", COLOR_EMERALD, COLOR_RESET);
        printf("%s+-----+---------------------------+------------------+--------+%s\n", COLOR_SEA_GREEN, COLOR_RESET);
        printf("%s| %-3s | %-25s | %-16s | %-6s |%s\n", COLOR_SEA_GREEN, "ID", "Nama Produk", "Harga", "Stok", COLOR_RESET);
        printf("%s+-----+---------------------------+------------------+--------+%s\n", COLOR_SEA_GREEN, COLOR_RESET);
        for (int i = 0; i < product_count && i < 8; i++) {
            printf("%s| %-3d | %-25.25s | Rp%-14.2f | %-6d |%s\n", COLOR_SEA_GREEN, products[i].id, products[i].name, products[i].price, products[i].stock, COLOR_RESET);
        }
        if(product_count > 8) printf("%s| ... | Tampilkan lebih banyak dengan fitur pencarian         |%s\n", COLOR_SEA_GREEN, COLOR_RESET);
        printf("%s+-----+---------------------------+------------------+--------+%s\n", COLOR_SEA_GREEN, COLOR_RESET);
        
        printf("\n%s[MENU UTAMA]%s\n", COLOR_EMERALD, COLOR_RESET);
        printf("%s1.%s Cari Produk       %s2.%s Keranjang Belanja   %s3.%s Wishlist Favorit\n", COLOR_LIME_GREEN, COLOR_RESET, COLOR_LIME_GREEN, COLOR_RESET, COLOR_LIME_GREEN, COLOR_RESET);
        printf("%s4.%s Riwayat Pesanan   %s5.%s Edit Profil         %s6.%s Keluar Sesi\n", COLOR_LIME_GREEN, COLOR_RESET, COLOR_LIME_GREEN, COLOR_RESET, COLOR_RED, COLOR_RESET);
        printf("\n%s=================================================================================%s\n", COLOR_PINE_GREEN, COLOR_RESET);
        printf("Masukkan pilihan Anda: ");
        choice = read_int_safe();
        switch (choice) {
            case 1: execution_search_menu(); break;
            case 2: manage_cart_menu(); break;
            case 3: manage_wishlist_menu(); break;
            case 4: view_customer_orders(); break;
            case 5: edit_profile_menu(); break;
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
                printf("\n%s[OK] Tindakan administrasi berhasil diterapkan!%s\n", COLOR_PINE_GREEN, COLOR_RESET);
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
                    printf("\n%s[OK] Kategori '%s' sukses diregistrasikan ke database.%s\n", COLOR_PINE_GREEN, new_cat, COLOR_RESET);
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
                printf("\n%s[OK] Produk berhasil dihapus dari inventaris sistem!%s\n", COLOR_PINE_GREEN, COLOR_RESET);
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
                printf("\n%s[OK] Kode promo voucher baru berhasil diaktifkan!%s\n", COLOR_PINE_GREEN, COLOR_RESET);
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
            
            // 1. VALIDASI USERNAME (Minimal 5 Karakter & Unik)
            while (true) {
                printf("Username Baru (Min 5 Karakter): ");
                read_string_safe(u.username, MAX_STR);
                if (strlen(u.username) < 5) {
                    printf("%s[!] Gagal: Username terlalu pendek! Minimal 5 karakter.%s\n\n", COLOR_RED, COLOR_RESET);
                    continue;
                }
                bool dup = false;
                for(int i = 0; i < user_count; i++) { 
                    if(strcmp(users[i].username, u.username) == 0) { dup = true; break; } 
                }
                if(dup) { 
                    printf("%s[!] Gagal: Username sudah terpakai! Coba yang lain.%s\n\n", COLOR_RED, COLOR_RESET); 
                    continue; 
                }
                break;
            }

            // 2. VALIDASI PASSWORD (Angka, Huruf Kecil, Kapital, & Tanda Baca)
            while (true) {
                printf("Password Baru                 : ");
                read_string_safe(u.password, MAX_STR);
                
                bool has_digit = false, has_lower = false, has_upper = false, has_punct = false;
                for (int i = 0; u.password[i] != '\0'; i++) {
                    if (isdigit(u.password[i])) has_digit = true;
                    else if (islower(u.password[i])) has_lower = true;
                    else if (isupper(u.password[i])) has_upper = true;
                    else if (ispunct(u.password[i])) has_punct = true; // Mengecek simbol/tanda baca seperti !,@,#,$,%, dll.
                }
                
                if (has_digit && has_lower && has_upper && has_punct) {
                    break;
                }
                printf("%s[!] Gagal: Password harus kombinasi dari Angka, Huruf Kecil, Kapital, dan minimal 1 Tanda Baca!%s\n\n", COLOR_RED, COLOR_RESET);
            }

            // 3. INPUT NAMA LENGKAP
            while (true) {
                printf("Nama Lengkap                 : ");
                read_string_safe(u.full_name, MAX_STR);
                if (strlen(u.full_name) > 0) break;
                printf("%s[!] Gagal: Nama lengkap tidak boleh kosong!%s\n\n", COLOR_RED, COLOR_RESET);
            }

            // 4. VALIDASI TEMPAT & TANGGAL LAHIR (Format DD-MM-YYYY)
            char tempat[MAX_STR], tgl[MAX_STR];
            while (true) {
                printf("Tempat Lahir                 : ");
                read_string_safe(tempat, MAX_STR);
                if (strlen(tempat) > 0) break;
                printf("%s[!] Gagal: Tempat lahir tidak boleh kosong!%s\n\n", COLOR_RED, COLOR_RESET);
            }
            
            while (true) {
                printf("Tanggal Lahir (DD-MM-YYYY)   : ");
                read_string_safe(tgl, MAX_STR);
                
                // Cek panjang karakter standar format DD-MM-YYYY harus 10 karakter
                if (strlen(tgl) == 10 && tgl[2] == '-' && tgl[5] == '-') {
                    bool valid_digits = true;
                    for (int i = 0; i < 10; i++) {
                        if (i == 2 || i == 5) continue;
                        if (!isdigit(tgl[i])) { valid_digits = false; break; }
                    }
                    if (valid_digits) {
                        int day = (tgl[0] - '0') * 10 + (tgl[1] - '0');
                        int month = (tgl[3] - '0') * 10 + (tgl[4] - '0');
                        if (day >= 1 && day <= 31 && month >= 1 && month <= 12) {
                            break; // Validasi sukses
                        }
                    }
                }
                printf("%s[!] Gagal: Gunakan format tanggal DD-MM-YYYY yang valid! (Contoh: 17-08-1945)%s\n\n", COLOR_RED, COLOR_RESET);
            }
            snprintf(u.birth_info, MAX_STR, "%s, %s", tempat, tgl); // Disatukan menjadi "Kota, DD-MM-YYYY"

            // 5. INPUT ALAMAT PENGIRIMAN
            while (true) {
                printf("Alamat Rumah                 : ");
                read_string_safe(u.address, MAX_STR);
                if (strlen(u.address) > 0) break;
                printf("%s[!] Gagal: Alamat tidak boleh kosong!%s\n\n", COLOR_RED, COLOR_RESET);
            }

            // 6. VALIDASI ALAMAT EMAIL (Harus mengandung '@' dan '.com')
            while (true) {
                printf("Alamat Email                 : ");
                read_string_safe(u.email, MAX_STR);
                char *at = strchr(u.email, '@');
                char *dotcom = strstr(u.email, ".com");
                
                // Pastikan karakter '@' ada, '.com' ada, dan posisi '@' berada sebelum '.com'
                if (at != NULL && dotcom != NULL && at < dotcom) {
                    break;
                }
                printf("%s[!] Gagal: Format email salah! Wajib memiliki karakter '@' dan berakhiran '.com'.%s\n\n", COLOR_RED, COLOR_RESET);
            }

            // 7. VALIDASI KODE NEGARA & NOMOR TELEPON (Tanpa Angka 0 di Depan)
            char cc[10], phone_num[MAX_STR];
            while (true) {
                printf("Masukkan Kode Negara (Contoh: 62): ");
                read_string_safe(cc, 10);
                bool is_num = true;
                for(int i = 0; cc[i] != '\0'; i++) { if(!isdigit(cc[i])) is_num = false; }
                if(strlen(cc) > 0 && is_num) break;
                printf("%s[!] Gagal: Kode negara harus diisi berupa angka!%s\n\n", COLOR_RED, COLOR_RESET);
            }

            while (true) {
                printf("Nomor Telepon (Tanpa angka 0 di depan): ");
                read_string_safe(phone_num, MAX_STR);
                
                if (phone_num[0] == '0') {
                    printf("%s[!] Gagal: Jangan masukkan angka '0' di awal nomor telepon!%s\n\n", COLOR_RED, COLOR_RESET);
                    continue;
                }
                bool is_num = true;
                for(int i = 0; phone_num[i] != '\0'; i++) { if(!isdigit(phone_num[i])) is_num = false; }
                if(strlen(phone_num) >= 5 && is_num) break;
                
                printf("%s[!] Gagal: Nomor telepon harus berupa angka dan minimal 5 digit!%s\n\n", COLOR_RED, COLOR_RESET);
            }
            snprintf(u.phone, MAX_STR, "+%s%s", cc, phone_num); // Disatukan menjadi gabungan format "+62812xxx"

            // SIMPAN DATA JIKA LOLOS SEMUA VALIDASI
            if (user_count < MAX_DATA) { 
                users[user_count++] = u; save_data(); 
                printf("\n%s[OK] Pendaftaran berhasil! Silakan masuk menggunakan akun baru Anda.%s\n", COLOR_PINE_GREEN, COLOR_RESET); 
            }
            press_enter_to_continue();
        }
    } while (choice != 3);
    
    if (root_product_name != NULL) free_trie(root_product_name);
    if (root_brand != NULL) free_trie(root_brand);
    if (root_category != NULL) free_trie(root_category);
    printf("\nTerima kasih telah menggunakan layanan kami. Sampai jumpa!\n");
    return 0;
}