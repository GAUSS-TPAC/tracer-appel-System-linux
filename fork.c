#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <unistd.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <pthread.h>

static int running = 1;  // Flag global pour arrêter le traçage

// Fonction pour tracer les appels système
void *trace_syscalls(void *arg) {
    pid_t pid = *((pid_t *)arg);
    int status;
    struct user_regs_struct regs;

    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) == -1) {
        perror("Erreur lors de PTRACE_ATTACH");
        return NULL;
    }

    waitpid(pid, &status, 0); // Attendre que le processus soit arrêté

    while (running) {
        if (ptrace(PTRACE_SYSCALL, pid, NULL, NULL) == -1) {
            perror("Erreur lors de PTRACE_SYSCALL");
            break;
        }
        waitpid(pid, &status, 0);

        if (WIFEXITED(status)) {
            printf("Processus terminé\n");
            break;
        }

        if (ptrace(PTRACE_GETREGS, pid, NULL, &regs) == -1) {
            perror("Erreur lors de PTRACE_GETREGS");
            break;
        }

        printf("Appel système : %lld\n", regs.orig_rax);
    }

    ptrace(PTRACE_DETACH, pid, NULL, NULL); // Détacher le processus
    return NULL;
}

// Fonction appelée lors du clic sur le bouton "Démarrer"
void on_start_button_clicked(GtkButton *button, gpointer user_data) {
    const gchar *pid_text = gtk_entry_get_text(GTK_ENTRY(user_data));

    if (pid_text == NULL || *pid_text == '\0') {
        g_print("Veuillez entrer un PID.\n");
        return;
    }

    pid_t pid = atoi(pid_text);
    if (pid <= 0) {
        g_print("PID invalide : %s\n", pid_text);
        return;
    }

    g_print("Traçage des appels système du PID : %d\n", pid);

    // Lancer le traçage dans un thread
    pthread_t trace_thread;
    pthread_create(&trace_thread, NULL, trace_syscalls, &pid);
    pthread_detach(trace_thread);  // Détacher le thread pour ne pas le bloquer
}

// Fonction appelée lors du clic sur le bouton "Arrêter"
void on_stop_button_clicked(GtkButton *button, gpointer user_data) {
    running = 0;  // Arrêter le traçage
    g_print("Arrêt du traçage des appels système.\n");
}

int main(int argc, char *argv[]) {
    GtkWidget *window, *grid, *label, *entry, *start_button, *stop_button;

    gtk_init(&argc, &argv);

    // Créer une fenêtre
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Traceur d'appels système");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 200);

    // Créer une grille
    grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(window), grid);

    // Ajouter une étiquette pour les instructions
    label = gtk_label_new("Veuillez entrer le PID du processus à tracer :");
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

    // Zone d'entrée pour le PID
    entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Entrez le PID");
    gtk_grid_attach(GTK_GRID(grid), entry, 0, 1, 1, 1);

    // Bouton pour démarrer
    start_button = gtk_button_new_with_label("Démarrer");
    gtk_grid_attach(GTK_GRID(grid), start_button, 0, 2, 1, 1);

    // Bouton pour arrêter
    stop_button = gtk_button_new_with_label("Arrêter");
    gtk_grid_attach(GTK_GRID(grid), stop_button, 0, 3, 1, 1);

    // Connecter les boutons à leurs fonctions respectives
    g_signal_connect(start_button, "clicked", G_CALLBACK(on_start_button_clicked), entry);
    g_signal_connect(stop_button, "clicked", G_CALLBACK(on_stop_button_clicked), NULL);

    // Gérer la fermeture de la fenêtre
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
