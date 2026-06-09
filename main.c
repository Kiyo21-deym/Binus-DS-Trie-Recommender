#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#define MAX_STR 100
#define MAX_DATA 200
#define ALPHABET_SIZE 128
#define SCREEN_WIDTH 80

/* ============================================================================
   ANSI COLOR CODES & STYLING (TOKOPEDIA GREEN & WHITE THEME)
   ============================================================================ */
#define COLOR_RESET   "\x1b[0m"
#define COLOR_GREEN   "\x1b[32m"     /* Hijau Tokopedia */
#define COLOR_B_GREEN "\x1b[1;32m"   /* Hijau Bold */
#define COLOR_WHITE   "\x1b[37m"     /* Putih */
#define COLOR_B_WHITE "\x1b[1;37m"   /* Putih Bold */
#define COLOR_RED     "\x1b[31m"     /* Merah Aksentuasi */

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
    char comment[255];
} Review;

typedef struct TrieNode {
    struct TrieNode *children[ALPHABET_SIZE];
    bool is_end_of_word;
    int product_ids[MAX_DATA];
    int product_count;
    int global_search_count;
} TrieNode;

/* ============================================================================
   GLOBAL MEMORY STATE
   ============================================================================ */
User users[MAX_DATA]; int user_count = 0;
Product products[MAX_DATA]; int product_count = 0;
Cart carts[MAX_DATA]; int cart_count = 0;
Wishlist wishlists[MAX_DATA]; int wishlist_count = 0;
Order orders[MAX_DATA]; int order_count = 0;

int logged_in_user_idx = -1;
TrieNode *root_product_name, *root_brand, *root_category;

/* ============================================================================
   UI & ALIGNMENT UTILITIES
   ============================================================================ */
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

/* Sapaan hangat top bar rata kiri setelah login (RevisI No. 2 & 3) */
void print_warm_welcome_bar(const char *username, const char *role) {
    printf("%s================================================================================%s\n", COLOR_GREEN, COLOR_RESET);
    printf("%sHallo, %s! Selamat datang kembali di Toko kami (%s).%s\n", COLOR_B_GREEN, username, role, COLOR_RESET);
    printf("%sAda yang bisa kami bantu hari ini?%s\n", COLOR_WHITE);
    printf("%s================================================================================%s\n", COLOR_GREEN, COLOR_RESET);
}

void print_ascii_logo() {
    printf("\n");
    print_centered_color("  _____ ___  _  _____  ____  _____ ____   ____ _____ ", COLOR_GREEN);
    print_centered_color(" |_   _/ _ \\| |/ / _ \\|  _ \\| ____|  _ \\ / ___| ____|", COLOR_GREEN);
    print_centered_color("   | || | | | ' / | | | |_) |  _| | |_) | |   |  _|  ", COLOR_GREEN);
    print_centered_color("   | || |_| | . \\ |_| |  __/| |___|  _ <| |___| |___ ", COLOR_GREEN);
    print_centered_color("   |_| \\___/|_|\\_\\___/|_|   |_____|_| \\_\\\\____|_____|", COLOR_GREEN);
    printf("\n");
    print_centered_color(">>> Sesi Interaktif Belanja Berbasis Algoritma Tries <<<", COLOR_B_WHITE);
    printf("\n");
}

/* ============================================================================
   TRIE DATA STRUCTURE LOGIC (SUPPORT SEMBARANG KEYWORD / SUBSTRING WORD MATCH)
   ============================================================================ */
TrieNode *create_trie_node() {
    TrieNode *node = (TrieNode *)malloc(sizeof(TrieNode));
    node->is_end_of_word = false; node->product_count = 0; node->global_search_count = 0;
    for (int i = 0; i < ALPHABET_SIZE; i++) node->children[i] = NULL;
    return node;
}

void to_lowercase(char *dest, const char *src) {
    int i = 0;
    while (src[i]) { dest[i] = tolower((unsigned char)src[i]); i++; }
    dest[i] = '\0';
}

void insert_trie(TrieNode *root, const char *keyword, int product_id) {
    char lower_keyword[MAX_STR]; to_lowercase(lower_keyword, keyword);
    TrieNode *current = root; int len = strlen(lower_keyword);
    for (int i = 0; i < len; i++) {
        int index = (int)lower_keyword[i];
        if (index < 0 || index >= ALPHABET_SIZE) continue;
        if (current->children[index] == NULL) current->children[index] = create_trie_node();
        current = current->children[index];
    }
    current->is_end_of_word = true;
    
    /* Cek agar ID produk tidak duplikat di node yang sama */
    for(int i = 0; i < current->product_count; i++) {
        if(current->product_ids[i] == product_id) return;
    }
    current->product_ids[current->product_count++] = product_id;
}

/* Fungsi memecah kalimat menjadi token kata agar bisa dicari secara acak (RevisI No. 4) */
void insert_trie_tokens(TrieNode *root, const char *phrase, int product_id) {
    char temp[MAX_STR];
    strcpy(temp, phrase);
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
            if (!dup) results[(*res_count)++] = node->product_ids[i];
        }
    }
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (node->children[i] != NULL) collect_all_words(node->children[i], results, res_count);
    }
}

int search_trie_prefix(TrieNode *root, const char *prefix, int *matched_ids) {
    char lower_prefix[MAX_STR]; to_lowercase(lower_prefix, prefix);
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

void rebuild_trives() {
    root_product_name = create_trie_node(); 
    root_brand = create_trie_node(); 
    root_category = create_trie_node();
    for (int i = 0; i < product_count; i++) {
        /* Memasukkan nama lengkap dan potongan kata agar fleksibel */
        insert_trie(root_product_name, products[i].name, products[i].id);
        insert_trie_tokens(root_product_name, products[i].name, products[i].id);
        
        insert_trie(root_brand, products[i].brand, products[i].id);
        insert_trie_tokens(root_brand, products[i].brand, products[i].id);
        
        insert_trie(root_category, products[i].category, products[i].id);
        insert_trie_tokens(root_category, products[i].category, products[i].id);
    }
}

/* ============================================================================
   CSV FILE MANAGEMENT
   ============================================================================ */
void load_data() {
    FILE *f; char line[500];
    if ((f = fopen("users.csv", "r"))) {
        fgets(line, sizeof(line), f); user_count = 0;
        while (fgets(line, sizeof(line), f)) {
            sscanf(line, "%d,%[^,],%[^,],%[^,],%s", &users[user_count].id, users[user_count].username, users[user_count].password, users[user_count].role, users[user_count].status);
            user_count++;
        } fclose(f);
    }
    if ((f = fopen("products.csv", "r"))) {
        fgets(line, sizeof(line), f); product_count = 0;
        while (fgets(line, sizeof(line), f)) {
            sscanf(line, "%d,%[^,],%[^,],%[^,],%lf,%d,%d,%d", &products[product_count].id, products[product_count].name, products[product_count].brand, products[product_count].category, &products[product_count].price, &products[product_count].stock, &products[product_count].seller_id, &products[product_count].search_count);
            product_count++;
        } fclose(f);
    }
    if ((f = fopen("carts.csv", "r"))) {
        fgets(line, sizeof(line), f); cart_count = 0;
        while (fgets(line, sizeof(line), f)) {
            sscanf(line, "%d,%d,%d", &carts[cart_count].user_id, &carts[cart_count].product_id, &carts[cart_count].quantity);
            cart_count++;
        } fclose(f);
    }
    if ((f = fopen("wishlists.csv", "r"))) {
        fgets(line, sizeof(line), f); wishlist_count = 0;
        while (fgets(line, sizeof(line), f)) {
            sscanf(line, "%d,%d", &wishlists[wishlist_count].user_id, &wishlists[wishlist_count].product_id);
            wishlist_count++;
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
}

/* ============================================================================
   CORE BUSINESS LOGIC
   ============================================================================ */
void add_to_cart(int product_id, int qty) {
    int uid = users[logged_in_user_idx].id;
    bool found = false;
    for (int i = 0; i < cart_count; i++) {
        if (carts[i].user_id == uid && carts[i].product_id == product_id) {
            carts[i].quantity += qty;
            found = true; break;
        }
    }
    if (!found) {
        carts[cart_count].user_id = uid;
        carts[cart_count].product_id = product_id;
        carts[cart_count].quantity = qty;
        cart_count++;
    }
    save_data();
    printf("%sProduk berhasil ditambahkan ke Keranjang Belanja!%s\n", COLOR_GREEN, COLOR_RESET);
}

void view_cart() {
    clear_screen(); 
    print_warm_welcome_bar(users[logged_in_user_idx].username, "Customer - Keranjang");
    int uid = users[logged_in_user_idx].id;
    double grand_total = 0;
    bool empty = true;

    printf("\n--- DAFTAR KERANJANG BELANJA ANDA ---\n");
    printf("+----+------------------------------+--------+--------------------+\n");
    printf("| ID | Nama Produk                  | Jumlah | Subtotal           |\n");
    printf("+----+------------------------------+--------+--------------------+\n");
    for (int i = 0; i < cart_count; i++) {
        if (carts[i].user_id == uid) {
            for (int j = 0; j < product_count; j++) {
                if (products[j].id == carts[i].product_id) {
                    empty = false;
                    double subtotal = products[j].price * carts[i].quantity;
                    grand_total += subtotal;
                    printf("| %-2d | %-28s | %-6d | Rp%-16.0f |\n", products[j].id, products[j].name, carts[i].quantity, subtotal);
                }
            }
        }
    }
    if (empty) {
        printf("|    | %-54s |\n", "Keranjang Anda kosong.");
    }
    printf("+----+------------------------------+--------+--------------------+\n");
    printf("Total Pembayaran : Rp%.0f\n", grand_total);
    printf("--------------------------------------------------------------------------------\n");
    printf("Tekan [Enter] untuk kembali...");
    getchar(); getchar();
}

/* Integrasi Pencarian berdasarkan tipe & penambahan langsung ke Cart (RevisI No. 4, 5, 6, 7 & 8) */
void execution_search_menu() {
    clear_screen();
    print_warm_welcome_bar(users[logged_in_user_idx].username, "Customer - Search Engine");
    
    printf("Pilih Kategori Kriteria Pencarian:\n");
    printf(" 1. Cari Berdasarkan Nama Produk\n");
    printf(" 2. Cari Berdasarkan Merek (Brand)\n");
    printf(" 3. Cari Berdasarkan Kategori\n");
    printf("--------------------------------------------------------------------------------\n");
    printf("Pilihan Anda : ");
    int type;
    scanf("%d", &type);
    
    printf("Kata Kunci   : ");
    char keyword[MAX_STR];
    scanf(" %[^\n]", keyword);
    
    TrieNode *target_root = root_product_name;
    if (type == 2) target_root = root_brand;
    else if (type == 3) target_root = root_category;
    
    int matched_ids[MAX_DATA];
    int total = search_trie_prefix(target_root, keyword, matched_ids);
    
    printf("\n%sHASIL PENCARIAN TRIES ENGINE (TABEL RATAKIRI):%s\n", COLOR_B_GREEN, COLOR_RESET);
    if (total == 0) {
        printf("Produk tidak ditemukan dengan kata kunci '%s'.\n", keyword);
        printf("--------------------------------------------------------------------------------\n");
        printf("Tekan [Enter] untuk kembali...");
        getchar(); getchar();
        return;
    }
    
    /* Tabel hasil pencarian rapi rata kiri */
    printf("+----+------------------------------+------------+------------+---------------+------+\n");
    printf("| ID | Nama Produk                  | Merek      | Kategori   | Harga         | Stok |\n");
    printf("+----+------------------------------+------------+------------+---------------+------+\n");
    for (int i = 0; i < total; i++) {
        for (int j = 0; j < product_count; j++) {
            if (products[j].id == matched_ids[i]) {
                products[j].search_count++;
                printf("| %-2d | %-28s | %-10s | %-10s | Rp%-11.0f | %-4d |\n", 
                       products[j].id, products[j].name, products[j].brand, products[j].category, products[j].price, products[j].stock);
                break;
            }
        }
    }
    printf("+----+------------------------------+------------+------------+---------------+------+\n");
    
    /* Proses Add Item to Cart Langsung Setelah Hasil Pencarian Muncul */
    printf("\nApakah ingin memasukkan produk ke keranjang? (y/n) : ");
    char ans;
    scanf(" %c", &ans);
    if (ans == 'y' || ans == 'Y') {
        int pid, qty;
        printf("Masukkan ID Produk : ");
        scanf("%d", &pid);
        printf("Masukkan Jumlah    : ");
        scanf("%d", &qty);
        
        /* Validasi apakah ID tersebut ada pada hasil pencarian */
        bool found_in_search = false;
        for(int i = 0; i < total; i++) {
            if(matched_ids[i] == pid) { found_in_search = true; break; }
        }
        
        if (found_in_search) {
            add_to_cart(pid, qty);
        } else {
            printf("%sGagal: ID Produk yang dimasukkan tidak ada dalam hasil pencarian!%s\n", COLOR_RED, COLOR_RESET);
        }
        printf("--------------------------------------------------------------------------------\n");
        printf("Tekan [Enter] untuk kembali ke menu...");
        getchar(); getchar();
    }
}

/* ============================================================================
   ROLE DASHBOARDS (RATAKIRI MODE)
   ============================================================================ */
void customer_menu() {
    int choice;
    do {
        clear_screen();
        print_warm_welcome_bar(users[logged_in_user_idx].username, "Customer");
        printf("1. Cari & Belanja Produk\n");
        printf("2. Lihat Keranjang Belanja\n");
        printf("%s3. Keluar dari Sesi Belanja (Logout)%s\n", COLOR_RED, COLOR_RESET);
        printf("--------------------------------------------------------------------------------\n");
        printf("Pilihan Menu : ");
        scanf("%d", &choice);

        if (choice == 1) execution_search_menu();
        else if (choice == 2) view_cart();
    } while (choice != 3);
    logged_in_user_idx = -1;
}

void seller_menu() {
    int choice;
    do {
        clear_screen();
        print_warm_welcome_bar(users[logged_in_user_idx].username, "Seller");
        printf("1. Tambah Katalog Produk Baru\n");
        printf("%s2. Keluar Sesi%s\n", COLOR_RED, COLOR_RESET);
        printf("--------------------------------------------------------------------------------\n");
        printf("Pilihan Menu : ");
        scanf("%d", &choice);

        if (choice == 1) {
            Product p;
            p.id = product_count > 0 ? products[product_count - 1].id + 1 : 1;
            p.seller_id = users[logged_in_user_idx].id; p.search_count = 0;
            
            printf("Nama Produk  : ");
            scanf(" %[^\n]", p.name);
            printf("Merek (Brand): ");
            scanf(" %[^\n]", p.brand);
            printf("Kategori     : ");
            scanf(" %[^\n]", p.category);
            printf("Harga        : ");
            scanf("%lf", &p.price);
            printf("Stok         : ");
            scanf("%d", &p.stock);
            
            products[product_count++] = p; save_data(); rebuild_trives();
            printf("%sKatalog produk berhasil diperbarui!%s\n", COLOR_GREEN, COLOR_RESET); 
            printf("Tekan [Enter] untuk melanjutkan...");
            getchar(); getchar();
        }
    } while (choice != 2);
    logged_in_user_idx = -1;
}

void admin_menu() {
    int choice;
    do {
        clear_screen();
        print_warm_welcome_bar(users[logged_in_user_idx].username, "Administrator");
        printf("1. Lihat Daftar Seluruh Pengguna Sistem\n");
        printf("%s2. Keluar Sesi%s\n", COLOR_RED, COLOR_RESET);
        printf("--------------------------------------------------------------------------------\n");
        printf("Pilihan Menu : ");
        scanf("%d", &choice);

        if (choice == 1) {
            clear_screen();
            print_warm_welcome_bar(users[logged_in_user_idx].username, "Admin - User Database");
            printf("\n--- DATA ROSTER PENGGUNA PLATFORM ---\n");
            for (int i = 0; i < user_count; i++) {
                printf("[-] ID: %-2d | User: %-12s | Hak Akses: %-8s | Status: %s\n", 
                       users[i].id, users[i].username, users[i].role, users[i].status);
            }
            printf("--------------------------------------------------------------------------------\n");
            printf("Tekan [Enter] untuk kembali...");
            getchar(); getchar();
        }
    } while (choice != 2);
    logged_in_user_idx = -1;
}

void execution_login() {
    clear_screen();
    print_ascii_logo();
    printf("--- GERBANG SISTEM AUTENTIKASI ---\n");
    char username[MAX_STR], password[MAX_STR];
    printf("Username : ");
    scanf("%s", username);
    printf("Password : ");
    scanf("%s", password);
    
    bool found = false;
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0 && strcmp(users[i].password, password) == 0) {
            if (strcmp(users[i].status, "BANNED") == 0) {
                printf("%sAkses Ditolak: Akun Anda sedang dibekukan.%s\n", COLOR_RED, COLOR_RESET); 
                printf("Tekan [Enter] untuk kembali..."); getchar(); getchar(); return;
            }
            logged_in_user_idx = i; found = true; break;
        }
    }
    
    if (!found) { 
        printf("%sKombinasi sandi / identitas salah!%s\n", COLOR_RED, COLOR_RESET); 
        printf("Tekan [Enter] untuk kembali..."); getchar(); getchar(); 
    } else {
        if (strcmp(users[logged_in_user_idx].role, "ADMIN") == 0) admin_menu();
        else if (strcmp(users[logged_in_user_idx].role, "SELLER") == 0) seller_menu();
        else customer_menu();
    }
}

/* ============================================================================
   MAIN FUNCTION ENTRY POINT
   ============================================================================ */
int main() {
    load_data();
    int choice;
    do {
        clear_screen(); 
        print_ascii_logo();
        
        printf("--- PORTAL UTAMA MASUK ---\n");
        printf("1. Masuk Sesi (Login)\n");
        printf("2. Daftarkan Akun Baru (Register)\n");
        printf("%s3. Keluar Aplikasi%s\n", COLOR_RED, COLOR_RESET);
        printf("--------------------------------------------------------------------------------\n");
        
        printf("Pilihan Menu : "); 
        scanf("%d", &choice);
        
        if (choice == 1) {
            execution_login();
        } else if (choice == 2) {
            clear_screen();
            print_ascii_logo();
            printf("--- PENDAFTARAN AKUN CUSTOMER BARU ---\n");
            User u; u.id = user_count > 0 ? users[user_count - 1].id + 1 : 1;
            strcpy(u.role, "CUSTOMER"); strcpy(u.status, "ACTIVE");
            
            printf("Username : "); scanf("%s", u.username);
            printf("Password : "); scanf("%s", u.password);
            
            users[user_count++] = u; save_data();
            printf("%sAkun sukses dibuat! Silakan gunakan menu Login.%s\n", COLOR_GREEN, COLOR_RESET); 
            printf("Tekan [Enter] untuk melanjutkan...");
            getchar(); getchar();
        }
    } while (choice != 3);
    
    clear_screen(); 
    printf("%sSistem ditutup dengan aman. Terima kasih telah bertransaksi!%s\n", COLOR_GREEN, COLOR_RESET);
    return 0;
}