package swapper;

import java.util.ArrayList;

// Następnie używamy poznanych na zajęciach implementacji zadań, gdzie zamiast zwykłego
// semafora, używamy semafora zaimplementowanego za pomocą swappera
public class CzytelnicyPisarze {
    private static final int WRITERCOUNT = 100;
    private static final int READERCOUNT = 1000;

    private static int reading = 0, writing = 0, waitingReaders = 0, waitingWriters = 0;
    private static SwapperSemaphore mutex = new SwapperSemaphore(1);
    private static SwapperSemaphore readers = new SwapperSemaphore(0);
    private static SwapperSemaphore writers = new SwapperSemaphore(0);

    private static class Reader implements Runnable {

        private int id;

        public Reader(int id) {
            this.id = id;
        }

        private void read() {
            System.out.println(id + " czyta");
        }

        @Override
        public void run() {
            mutex.PS();

            if (writing + waitingWriters > 0) {
                waitingReaders++;
                mutex.VS();
                readers.PS();
                waitingReaders--;
            }

            if (reading == 0)
                System.out.println("Czytelnicy wchodzą do czytelni");

            reading++;

            if (waitingReaders > 0)
                readers.VS();
            else
                mutex.VS();

            read();
            mutex.PS();
            reading--;

            if (reading == 0)
                System.out.println("Czytelnicy wyszli z czytelni");

            if (reading == 0 && waitingWriters > 0)
                writers.VS();
            else
                mutex.VS();
        }
    }


    private static class Writer implements Runnable {
        private int id;

        public Writer(int id) {
            this.id = id;
        }

        private void write() {
            System.out.println(id + " pisze");
        }

        @Override
        public void run() {
            mutex.PS();

            if (writing + reading > 0) {
               waitingWriters++;
               mutex.VS();
               writers.PS();
               waitingWriters--;
            }

            System.out.println("Pisarz wchodzi do czytelni");
            writing++;
            mutex.VS();
            write();
            mutex.PS();
            writing--;
            System.out.println(("Pisarz wyszedł z czytelni"));

            if (waitingReaders > 0)
                readers.VS();
            else if (waitingWriters > 0)
                writers.VS();
            else
                mutex.VS();
        }
    }

    public static void main(String[] args) {
        ArrayList<Thread> threads = new ArrayList<>();
        for (int i = 0; i < WRITERCOUNT; i++) {
            Thread t = new Thread(new Writer(i));
            threads.add(t);
        }

        for (int i = 0; i < READERCOUNT; i++) {
            Thread t = new Thread(new Reader(i));
            threads.add(t);
        }

        for (Thread t: threads)
            t.start();

        try {
            for (Thread t: threads)
                t.join();
        }catch (InterruptedException e) {
            System.out.println("Wątek główny przerwany");
        }
    }
}
