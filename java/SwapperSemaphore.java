package swapper;

import java.util.ArrayList;

// Najpierw używamy Swappera do stworzenia semafora ogólnego nazwanego SwapperSemaphore
public class SwapperSemaphore {
    private Swapper<Integer> mutex = new Swapper();
    private Swapper<Integer> first = new Swapper();
    private Swapper<Integer> others = new Swapper();
    private int state;
    private boolean waiting;

    private ArrayList<Integer> emptyList = new ArrayList<>();
    private ArrayList<Integer> nonEmptyList = new ArrayList<>();

    public SwapperSemaphore(int state) {
        this.state = state;
        nonEmptyList.add(1);
        try {
            mutex.swap(emptyList, nonEmptyList);
            others.swap(emptyList, nonEmptyList);
        }catch (InterruptedException e) {
            System.out.println("Wątek przerwany");
        }
    }

    public void PS() {
        try {
            others.swap(nonEmptyList, emptyList);
            mutex.swap(nonEmptyList, emptyList);
            if (state == 0) {
                waiting = true;
                mutex.swap(emptyList, nonEmptyList);
                first.swap(nonEmptyList, emptyList);
                waiting = false;
            }
            state--;
            mutex.swap(emptyList, nonEmptyList);
            others.swap(emptyList, nonEmptyList);
        } catch (InterruptedException e) {
            System.out.println("Wątek przerwany");
        }
    }

    public void VS() {
        try {
            mutex.swap(nonEmptyList, emptyList);
            state++;
            if (waiting)
                first.swap(emptyList, nonEmptyList);
            else
                mutex.swap(emptyList, nonEmptyList);

        }catch (InterruptedException e) {
            System.out.println("Wątek przerwany");
        }
    }

}
