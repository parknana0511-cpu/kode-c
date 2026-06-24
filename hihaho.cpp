#include <iostream>
#include <mysql.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <cstring>

using namespace std;

struct UserSession {
    string id_user;
    string username;
    string nama_lengkap;
    string role;
} session;

void clearScreen() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

void pauseScreen() {
    cout << "\nTekan ENTER untuk melanjutkan...";
    cin.ignore(1000, '\n');
    cin.get();
}

class Database {
private:
    MYSQL* conn;
public:
    Database() { conn = mysql_init(0); }

    bool connect() {
        conn = mysql_real_connect(conn, "127.0.0.1", "root", "", "perpustakaan_digital", 3308, NULL, 0);
        if (conn == NULL) {
            cout << "==================================================\n";
            cout << "[ERROR] Gagal Terhubung ke WAMP Server!\n";
            cout << "Pesan Sistem: " << mysql_error(conn) << "\n";
            cout << "==================================================\n";
            return false;
        }
        return true;
    }

    MYSQL* getConnection() { return conn; }
    ~Database() { mysql_close(conn); }
};

// ==================== 1. LOGIN SYSTEM ====================
bool loginSystem(Database& db) {
    string username, password;
    clearScreen();
    cout << "=========================================\n";
    cout << "   LOGIN SISTEM PERPUSTAKAAN DIGITAL     \n";
    cout << "=========================================\n";
    cout << "Username: "; cin >> username;
    cout << "Password: "; cin >> password;
    cout << "=========================================\n";

    stringstream query;
    query << "SELECT id_user, username, nama_lengkap, role FROM users "
          << "WHERE username='" << username << "' AND password=MD5('" + password + "')";

    if (mysql_query(db.getConnection(), query.str().c_str()) == 0) {
        MYSQL_RES* res = mysql_store_result(db.getConnection());
        MYSQL_ROW row = mysql_fetch_row(res);
        if (row) {
            session.id_user      = row[0];
            session.username     = row[1];
            session.nama_lengkap = row[2];
            session.role         = row[3];
            mysql_free_result(res);
            return true;
        }
        mysql_free_result(res);
    }
    cout << "Kredensial salah! Gagal masuk ke sistem.\n";
    pauseScreen();
    return false;
}

// ==================== 2. LIHAT KOLEKSI BUKU ====================
void tampilkanBuku(Database& db) {
    clearScreen();
    cout << "========================================================================================\n";
    cout << "                                   DAFTAR KOLEKSI BUKU                                  \n";
    cout << "========================================================================================\n";
    cout << setw(5) << left << "ID" 
         << setw(15) << left << "ISBN" 
         << setw(45) << left << "Judul Buku" 
         << setw(20) << left << "Penulis" 
         << setw(10) << left << "Stok" << endl;
    cout << "----------------------------------------------------------------------------------------\n";

    string query = "SELECT id_buku, isbn, judul, penulis, stok_tersedia FROM buku";
    if (mysql_query(db.getConnection(), query.c_str()) == 0) {
        MYSQL_RES* res = mysql_store_result(db.getConnection());
        MYSQL_ROW row;
while ((row = mysql_fetch_row(res))) {
    cout << left
         << setw(15) << row[0]
         << setw(20) << row[1]
         << setw(45) << row[2]
         << setw(18) << row[3]
         << setw(18) << row[4]
         << setw(15) << row[5]
         << endl;
}
        mysql_free_result(res);
    }
    cout << "========================================================================================\n";
    pauseScreen();
}

// ==================== 3. TAMBAH KOLEKSI BUKU BARU ====================
void tambahBuku(Database& db) {
    clearScreen();
    string isbn, judul, penulis;
    int stok;

    cout << "=========================================\n";
    cout << "        TAMBAH KOLEKSI BUKU BARU         \n";
    cout << "=========================================\n";
    cin.ignore(1000, '\n'); 
    cout << "Masukkan ISBN       : "; getline(cin, isbn);
    cout << "Masukkan Judul Buku : "; getline(cin, judul);
    cout << "Masukkan Penulis    : "; getline(cin, penulis);
    cout << "Masukkan Jumlah Stok: "; cin >> stok;

    stringstream query;
    query << "INSERT INTO buku (isbn, judul, penulis, stok_tersedia) VALUES ("
          << "'" << isbn << "', '" << judul << "', '" << penulis << "', " << stok << ")";

    if (mysql_query(db.getConnection(), query.str().c_str()) == 0) {
        cout << "\nSukses! Buku baru berhasil ditambahkan ke rak digital.\n";
    } else {
        cout << "\nGagal menambahkan buku! Pesan error: " << mysql_error(db.getConnection()) << endl;
    }
    pauseScreen();
}

// ==================== 4. TRANSAKSI PEMINJAMAN BUKU ====================
void transaksiPeminjaman(Database& db) {
    clearScreen();
    string id_anggota, id_buku, no_pinjam;
    cout << "=========================================\n";
    cout << "           TRANSAKSI PEMINJAMAN BUKU         \n";
    cout << "=========================================\n";
    cout << "Masukkan ID Anggota : "; cin >> id_anggota;
    cout << "Masukkan ID Buku    : "; cin >> id_buku;
    cout << "Masukkan No Pinjam  : "; cin >> no_pinjam; 

    mysql_query(db.getConnection(), "START TRANSACTION");

    stringstream cekStok;
    cekStok << "SELECT stok_tersedia FROM buku WHERE id_buku = " << id_buku;
    mysql_query(db.getConnection(), cekStok.str().c_str());
    MYSQL_RES* resStok = mysql_store_result(db.getConnection());
    MYSQL_ROW rowStok = mysql_fetch_row(resStok);

    if (!rowStok || atoi(rowStok[0]) <= 0) {
        cout << "\n[Error] Stok buku habis atau buku tidak ditemukan!\n";
        mysql_free_result(resStok);
        mysql_query(db.getConnection(), "ROLLBACK"); 
        pauseScreen();
        return;
    }
    mysql_free_result(resStok);

    bool success = true;
    stringstream queryPinjam;
    queryPinjam << "INSERT INTO peminjaman (no_pinjam, id_anggota, id_buku, id_user, tanggal_pinjam, tgl_kembali_rencana, status) "
                << "VALUES ('" << no_pinjam << "', '" << id_anggota << "', " << id_buku << ", " << session.id_user << ", CURDATE(), DATE_ADD(CURDATE(), INTERVAL 7 DAY), 'dipinjam')";
    if (mysql_query(db.getConnection(), queryPinjam.str().c_str()) != 0) success = false;

    stringstream queryUpdateStok;
    queryUpdateStok << "UPDATE buku SET stok_tersedia = stok_tersedia - 1 WHERE id_buku = " << id_buku;
    if (mysql_query(db.getConnection(), queryUpdateStok.str().c_str()) != 0) success = false;

    if (success) {
        mysql_query(db.getConnection(), "COMMIT");
        cout << "\nTransaksi BERHASIL! Buku telah dipinjam.\n";
    } else {
        mysql_query(db.getConnection(), "ROLLBACK");
        cout << "\nTransaksi GAGAL! Terjadi kesalahan sistem.\n";
    }
    pauseScreen();
}

// ==================== 5. TRANSAKSI PENGEMBALIAN BUKU ====================
void transaksiPengembalian(Database& db) {
    clearScreen();
    string no_pinjam, kondisi, keterangan;
    cout << "=========================================\n";
    cout << "        TRANSAKSI PENGEMBALIAN BUKU       \n";
    cout << "=========================================\n";
    cout << "Masukkan Nomor Peminjaman: "; cin >> no_pinjam;
    cout << "Kondisi Buku (1: baik, 2: rusak, 3: hilang): "; cin >> kondisi;
    cin.ignore(1000, '\n'); 
    cout << "Keterangan/Catatan: "; getline(cin, keterangan);
    
    string enum_kondisi = (kondisi == "2") ? "rusak" : (kondisi == "3") ? "hilang" : "baik";
    
    stringstream cekQuery;
    cekQuery << "SELECT id_pinjam, id_buku, tgl_kembali_rencana, DATEDIFF(CURDATE(), tgl_kembali_rencana) AS keterlambatan "
             << "FROM peminjaman WHERE no_pinjam = '" << no_pinjam << "' AND status = 'dipinjam'";
    
    if (mysql_query(db.getConnection(), cekQuery.str().c_str()) != 0) {
        cout << "Gagal melakukan query cek data.\n";
        pauseScreen();
        return;
    }
    
    MYSQL_RES* resCek = mysql_store_result(db.getConnection());
    MYSQL_ROW rowCek = mysql_fetch_row(resCek);
    if (!rowCek) {
        cout << "\n[Error] Nomor transaksi tidak valid atau buku sudah dikembalikan!\n";
        mysql_free_result(resCek);
        pauseScreen();
        return;
    }
    
    string id_pinjam = rowCek[0];
    string id_buku   = rowCek[1];
    int selisih_hari = (rowCek[3] != NULL) ? atoi(rowCek[3]) : 0;
    mysql_free_result(resCek);
    
    mysql_query(db.getConnection(), "START TRANSACTION");
    bool success = true;
    
    stringstream insKembali;
    insKembali << "INSERT INTO pengembalian (id_pinjam, id_user, tgl_kembali, kondisi_buku, keterangan) "
               << "VALUES (" << id_pinjam << ", " << session.id_user << ", CURDATE(), '" << enum_kondisi << "', '" << keterangan << "')";
    if (mysql_query(db.getConnection(), insKembali.str().c_str()) != 0) success = false;
    
    string status_akhir = (selisih_hari > 0) ? "terlambat" : "dikembalikan";
    stringstream updPinjam;
    updPinjam << "UPDATE peminjaman SET status = '" << status_akhir << "', tgl_kembali_aktual = CURDATE() WHERE id_pinjam = " << id_pinjam;
    if (mysql_query(db.getConnection(), updPinjam.str().c_str()) != 0) success = false;

    if (enum_kondisi != "hilang") {
        stringstream updStok;
        updStok << "UPDATE buku SET stok_tersedia = stok_tersedia + 1 WHERE id_buku = " << id_buku;
        if (mysql_query(db.getConnection(), updStok.str().c_str()) != 0) success = false;
    }
    
int total_denda = 0;

// denda keterlambatan
if (selisih_hari > 0)
    total_denda += selisih_hari * 1000;

// denda kondisi buku
if (enum_kondisi == "rusak")
    total_denda += 50000;

if (enum_kondisi == "hilang")
    total_denda += 100000;

if (total_denda > 0) {

    stringstream insDenda;

    insDenda << "INSERT INTO denda "
             << "(id_pinjam, jumlah_hari_terlambat, denda_per_hari, total_denda, status_bayar) "
             << "VALUES ("
             << id_pinjam << ", "
             << selisih_hari << ", "
             << "1000, "
             << total_denda << ", "
             << "'belum')";

    if (mysql_query(db.getConnection(), insDenda.str().c_str()) != 0)
        success = false;

    cout << "\n=== RINCIAN DENDA ===\n";
    cout << "Total Denda: Rp " << total_denda << endl;
}
    
    if (success) {
        mysql_query(db.getConnection(), "COMMIT");
        cout << "\nSukses! Data pengembalian berhasil diproses ke sistem.\n";
    } else {
        mysql_query(db.getConnection(), "ROLLBACK");
        cout << "\nGagal memproses pengembalian. Transaksi di-rollback.\n";
    }
    pauseScreen();
}

// ==================== 6. FITUR LAPORAN PEMINJAMAN & DENDA (BARU) ====================
void tampilkanLaporan(Database& db) {
    clearScreen();
    cout << "=========================================================================================================\n";
    cout << "                                      LAPORAN PEMINJAMAN & DENDA                                         \n";
    cout << "=========================================================================================================\n";
cout << left
     << setw(15) << "No Pinjam"
     << setw(20) << "ID Anggota"
     << setw(45) << "Judul Buku"
     << setw(18) << "Tgl Pinjam"
     << setw(18) << "Status"
     << setw(15) << "Total Denda"
     << endl;
cout << string(121, '-') << endl;

stringstream query;

query << "SELECT "
      << "p.no_pinjam, "
      << "p.id_anggota, "
      << "b.judul, "
      << "p.tanggal_pinjam, "
      << "p.status, "
      << "IFNULL(d.total_denda,0) "
      << "FROM peminjaman p "
      << "LEFT JOIN buku b ON p.id_buku = b.id_buku "
      << "LEFT JOIN denda d ON p.id_pinjam = d.id_pinjam";
      
if (mysql_query(db.getConnection(), query.str().c_str()) == 0) {
    MYSQL_RES* res = mysql_store_result(db.getConnection());

    cout << "\nJumlah data ditemukan: "
         << mysql_num_rows(res) << endl;

    MYSQL_ROW row;

while ((row = mysql_fetch_row(res))) {
    cout << setw(10) << left << row[0]
         << setw(15) << left << row[1]
         << setw(25) << left << row[2]
         << setw(15) << left << row[3]
         << setw(15) << left << row[4]
         << setw(15) << left << row[5]
         << endl;
}
    mysql_free_result(res);
} else {
        cout << "[Error] Gagal menarik data laporan: " << mysql_error(db.getConnection()) << endl;
    }
    cout << "=========================================================================================================\n";
    pauseScreen();
}

// ==================== ALUR MENU UTAMA ====================
void menuUtama(Database& db) {
    int pilihan;
    do {
        clearScreen();
        cout << "=========================================\n";
        cout << "     SISTEM PERPUSTAKAAN DIGITAL (C++)   \n";
        cout << "=========================================\n";
        cout << "Petugas: " << session.nama_lengkap << " (" << session.role << ")\n";
        cout << "-----------------------------------------\n";
        cout << "1. Lihat Koleksi Buku\n";
        cout << "2. Tambah Koleksi Buku Baru (Baru)\n";
        cout << "3. Transaksi Peminjaman Buku\n";
        cout << "4. Transaksi Pengembalian Buku (Revisi)\n";
        cout << "5. Lihat Laporan Peminjaman & Denda (Baru)\n"; // Menu Baru dimasukkan di sini
        cout << "6. Keluar\n";
        cout << "=========================================\n";
        cout << "Pilihan [1-6]: "; cin >> pilihan;
        
        switch (pilihan) {
            case 1: tampilkanBuku(db); break;
            case 2: tambahBuku(db); break; 
            case 3: transaksiPeminjaman(db); break;
            case 4: transaksiPengembalian(db); break;
            case 5: tampilkanLaporan(db); break;
            case 6: cout << "\nKeluar dari sistem. Selamat bekerja kembali!\n"; break;
            default: cout << "\nPilihan menu tidak valid!\n"; pauseScreen();
        }
    } while (pilihan != 6);
}

int main() {
    Database db;
    cout << "Sedang mencoba melakukan jembatan koneksi ke MySQL WAMP Server...\n";
    if (!db.connect()) {
        cout << "\nPastikan aplikasi WAMP Server berstatus HIJAU di taskbar.\n\nTekan ENTER untuk keluar...";
        cin.get(); 
        return 1;
    }
    
    cout << "Koneksi sukses! Menghubungkan ke sistem login...\nTekan ENTER untuk melanjutkan...";
    cin.get(); 

    if (loginSystem(db)) {
        menuUtama(db);
    } else {
        cout << "\nAkses ditolak. Program dihentikan.\n";
        cin.ignore();
        cin.get();
    }
    return 0;
}
