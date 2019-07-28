package swapper;

import java.util.ArrayList;
import java.util.Collection;
import java.util.concurrent.Semaphore;

public class Swapper<E> {

    private class Pair {
        Semaphore semaphore;
        Collection<E> removed;

        public Pair(Semaphore semaphore, Collection<E> removed) {
            this.semaphore = semaphore;
            this.removed = removed;
        }
    }

    private static Semaphore mutex;
    private boolean active;
    private ArrayList<E> elements;
    private ArrayList<Pair> semaphores;

    public Swapper() {
        mutex = new Semaphore(1);
        active = false;
        elements = new ArrayList<>();
        semaphores = new ArrayList<>();
    }

    public void addToArray(ArrayList<E> arrayList, E element) {
        if (!arrayList.contains(element))
            arrayList.add(element);
    }

    public void addAllToArray(ArrayList<E> elements, Collection<E> added) {
        for (E i: added)
            addToArray(elements, i);
    }


    private boolean eraseSemaphore(ArrayList<Pair> semaphores) {
        for (Pair i : semaphores) {
            if (elements.containsAll(i.removed)) {
                i.semaphore.release();
                semaphores.remove(i);
                return true;
            }
        }
        return false;
    }

    public void swap(Collection<E> removed, Collection<E> added) throws InterruptedException {
        mutex.acquire();

        if (active || !elements.containsAll(removed)) {
            Pair pair = new Pair(new Semaphore(0), removed);
            semaphores.add(pair);
            mutex.release();
            pair.semaphore.acquire();
            mutex.acquire();
        }

        else
            active = true;

        ArrayList<E> elementsCopy = new ArrayList<>();
        elementsCopy.addAll(elements);
        Thread current = Thread.currentThread();
        elements.removeAll(removed);
        addAllToArray(elements, added);

        if (current.isInterrupted()) {
            elements = elementsCopy;
            active = eraseSemaphore(semaphores);
            mutex.release();
            throw new InterruptedException();
        }

        active = eraseSemaphore(semaphores);
        mutex.release();
    }
}
