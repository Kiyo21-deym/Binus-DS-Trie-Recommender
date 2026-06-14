/* ============================================================================
   ADMINISTRATOR DASHBOARD CONTROL (FULL ASCII MURNI & BERWARNA)
   ============================================================================ */

// Helper function untuk mencetak judul halaman dengan box ASCII murni yang estetik
void print_admin_section_header(const char *title) {
    int len = strlen(title);
    printf("%s+", COLOR_DARK_GREEN);
    for (int i = 0; i < len + 4; i++) printf("-");
    printf("+%s\n", COLOR_FOREST_GREEN);
    
    printf("%s|  %s%s%s  |%s\n", COLOR_CYAN, COLOR_B_WHITE, title, COLOR_CYAN, COLOR_RESET);
    
    printf("%s+", COLOR_CYAN);
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

        printf("%sCari (Username/ID/Role) atau tekan [ENTER] untuk lewati: %s", COLOR_YELLOW, COLOR_RESET);
        read_string_safe(keyword, MAX_STR);

        printf("\n%s%5s | %-15s | %-10s | %-10s%s\n", COLOR_CYAN, "ID", "Username", "Role", "Status", COLOR_RESET);
        printf("%s------+-----------------+------------+-----------%s\n", COLOR_CYAN, COLOR_RESET);

        int count = 0;
        for (int i = 0; i < user_count; i++) {
            if (strlen(keyword) == 0 || strstr(users[i].username, keyword) || strstr(users[i].role, keyword)) {
                printf("%s%5d %s| %s%-15s %s| %s%-10s %s| %s%-10s%s\n", 
                       COLOR_YELLOW, users[i].id, COLOR_CYAN, 
                       COLOR_B_WHITE, users[i].username, COLOR_CYAN, 
                       COLOR_GREEN, users[i].role, COLOR_CYAN, 
                       COLOR_RED, users[i].status, COLOR_RESET);
                count++;
            }
        }
        
        if (count == 0) {
            printf("%s[ Tidak ada data pengguna yang cocok dengan pencarian ]%s\n", COLOR_RED, COLOR_RESET);
        }
        printf("%s----------------------------------------------------------%s\n", COLOR_CYAN, COLOR_RESET);
        printf("%s[0] %sKembali / Batal\n", COLOR_RED, COLOR_YELLOW);
        printf("%sMasukkan ID Pengguna untuk diproses: %s", COLOR_GREEN, COLOR_RESET);
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
                printf("\n%s[✔] Tindakan administrasi berhasil diterapkan!%s\n", COLOR_GREEN, COLOR_RESET);
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
        
        printf("%sDaftar Kategori Terdaftar Saat Ini:%s\n", COLOR_CYAN, COLOR_RESET);
        char seen[MAX_DATA][MAX_STR];
        int seen_count = 0;
        for (int i = 0; i < product_count; i++) {
            bool found = false;
            for (int j = 0; j < seen_count; j++) {
                if (strcmp(seen[j], products[i].category) == 0) { found = true; break; }
            }
            if (!found) {
                strcpy(seen[seen_count++], products[i].category);
                printf("%s  |-- %s%s%s\n", COLOR_CYAN, COLOR_YELLOW, products[i].category, COLOR_RESET);
            }
        }
        
        printf("%s----------------------------------%s\n", COLOR_CYAN, COLOR_RESET);
        if (strlen(error_msg) > 0) {
            printf("%s[!] %s%s\n\n", COLOR_RED, error_msg, COLOR_RESET);
            strcpy(error_msg, ""); 
        }

        printf("%s[1] %sTambah Kategori Baru\n", COLOR_CYAN, COLOR_YELLOW);
        printf("%s[2] %sKembali / Batal\n", COLOR_RED, COLOR_YELLOW);
        printf("%sPilihan Operasi: %s", COLOR_GREEN, COLOR_RESET);
        opt = read_int_safe();

        if (opt == 1) {
            char new_cat[MAX_STR];
            printf("%sNama Kategori Baru: %s", COLOR_GREEN, COLOR_RESET); 
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
                    printf("\n%s[✔] Kategori '%s' sukses diregistrasikan ke database.%s\n", COLOR_GREEN, new_cat, COLOR_RESET);
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

        printf("%s%5s | %-30s | %-20s%s\n", COLOR_CYAN, "ID", "Nama Produk", "Kategori", COLOR_RESET);
        printf("%s------+--------------------------------+---------------------%s\n", COLOR_CYAN, COLOR_RESET);
        for(int i = 0; i < product_count; i++) {
            printf("%s[%3d] %s| %s%-30s %s| %s%-20s%s\n", 
                   COLOR_YELLOW, products[i].id, COLOR_CYAN, 
                   COLOR_B_WHITE, products[i].name, COLOR_CYAN, 
                   COLOR_GREEN, products[i].category, COLOR_RESET);
        }
        printf("%s--------------------------------------------------------------%s\n", COLOR_CYAN, COLOR_RESET);
        
        printf("%s[0] %sKembali ke Menu Utama\n", COLOR_RED, COLOR_YELLOW);
        printf("%sMasukkan ID Produk yang ingin dihapus secara permanen: %s", COLOR_GREEN, COLOR_RESET);
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
                printf("\n%s[✔] Produk berhasil dihapus dari inventaris sistem!%s\n", COLOR_GREEN, COLOR_RESET);
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

        printf("%s%-15s | %-10s | %-15s | %-15s%s\n", COLOR_CYAN, "Kode Voucher", "Diskon %", "Maks Potongan", "Min Belanja", COLOR_RESET);
        printf("%s----------------+------------+-----------------+----------------%s\n", COLOR_CYAN, COLOR_RESET);
        for (int i = 0; i < voucher_count; i++) {
            printf("%s%-15s %s| %s%-10d %s| %sRp%-13.2f %s| %sRp%-13.2f%s\n", 
                   COLOR_B_WHITE, vouchers[i].voucher_code, COLOR_CYAN, 
                   COLOR_YELLOW, vouchers[i].discount_percent, COLOR_CYAN, 
                   COLOR_GREEN, vouchers[i].max_discount_amount, COLOR_CYAN, 
                   COLOR_GREEN, vouchers[i].min_purchase, COLOR_RESET);
        }
        printf("%s----------------------------------------------------------------%s\n", COLOR_CYAN, COLOR_RESET);
        
        printf("%s[1] %sRilis Voucher Baru\n", COLOR_CYAN, COLOR_YELLOW);
        printf("%s[2] %sKembali ke Menu Utama\n", COLOR_RED, COLOR_YELLOW);
        printf("%sPilihan Aksi: %s", COLOR_GREEN, COLOR_RESET);
        choice = read_int_safe();

        if (choice == 1) {
            if (voucher_count >= MAX_DATA) {
                strcpy(error_msg, "Alokasi penyimpanan database Voucher penuh!");
            } else {
                Voucher v;
                printf("%sKode Voucher Baru : %s", COLOR_GREEN, COLOR_RESET); read_string_safe(v.voucher_code, MAX_STR);
                printf("%sPotongan Diskon (%%): %s", COLOR_GREEN, COLOR_RESET); v.discount_percent = read_int_safe();
                printf("%sMaksimal Nominal   : Rp", COLOR_GREEN); v.max_discount_amount = read_double_safe();
                printf("%sMinimal Pembelian  : Rp", COLOR_GREEN); v.min_purchase = read_double_safe();
                strcpy(v.status, "ACTIVE");
                
                vouchers[voucher_count++] = v;
                save_data();
                printf("\n%s[✔] Kode promo voucher baru berhasil diaktifkan!%s\n", COLOR_GREEN, COLOR_RESET);
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

        printf("%s[1] %sBlokir Akses Pengguna\n", COLOR_CYAN, COLOR_YELLOW);
        printf("%s[2] %sHapus Data Pengguna\n", COLOR_CYAN, COLOR_YELLOW);
        printf("%s[3] %sHapus Produk Katalog\n", COLOR_CYAN, COLOR_YELLOW);
        printf("%s[4] %sKelola Kategori Produk\n", COLOR_CYAN, COLOR_YELLOW);
        printf("%s[5] %sPenerbitan Promo Voucher\n", COLOR_CYAN, COLOR_YELLOW);
        printf("%s[6] %sAkses Laporan Finansial Sistem\n", COLOR_CYAN, COLOR_YELLOW);
        printf("%s[7] %sKeluar Sesi (Logout System)\n", COLOR_RED, COLOR_YELLOW);
        printf("%sMasukkan Pilihan Operasi: %s", COLOR_GREEN, COLOR_RESET);
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
                printf("%s+------------------------------------------------------------+%s\n", COLOR_CYAN, COLOR_RESET);
                printf("%s| %sTotal Akumulasi Transaksi  : %sRp%-27.2f%s |\n", COLOR_CYAN, COLOR_YELLOW, COLOR_GREEN, g_rev, COLOR_CYAN);
                printf("%s| %sTotal Kuantitas Pesanan    : %s%-29d%s |\n", COLOR_CYAN, COLOR_YELLOW, COLOR_GREEN, order_count, COLOR_CYAN);
                printf("%s| %sTotal Registrasi Pengguna  : %s%-29d%s |\n", COLOR_CYAN, COLOR_YELLOW, COLOR_GREEN, user_count, COLOR_CYAN);
                printf("%s+------------------------------------------------------------+%s\n\n", COLOR_CYAN, COLOR_RESET);
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