package processor.exceptions;

/**
 * Created by lionell on 24.02.16.
 *
 * @author Ruslan Sakevych
 */
public class MemoryException extends RuntimeException {
    public MemoryException(String message) {
        super(message);
    }

    @Override
    public String getMessage() {
        return "Memory Error: " + super.getMessage();
    }
}
