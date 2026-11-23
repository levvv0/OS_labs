package main;

import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;
import org.springframework.amqp.AmqpException;
import org.springframework.amqp.rabbit.annotation.RabbitListener;
import org.springframework.amqp.rabbit.core.RabbitTemplate;
import org.springframework.http.HttpStatus;
import org.springframework.stereotype.Service;
import org.springframework.web.server.ResponseStatusException;

@Service
public class TaskService {
    private final RabbitTemplate rabbitTemplate;
    private final ConcurrentMap<String, TaskStatus> tasks = new ConcurrentHashMap<>();

    public TaskService(RabbitTemplate rabbitTemplate) {
        this.rabbitTemplate = rabbitTemplate;
    }

    public String submit(String text) {
        String id = UUID.randomUUID().toString();
        tasks.put(id, new TaskStatus("queued", null, null));
        try {
            rabbitTemplate.convertAndSend("word-tasks", id + "\t" + text);
        } catch (AmqpException error) {
            tasks.remove(id);
            throw new ResponseStatusException(HttpStatus.SERVICE_UNAVAILABLE, "RabbitMQ unavailable", error);
        }
        return id;
    }

    public TaskStatus get(String id) {
        TaskStatus status = tasks.get(id);
        if (status == null) throw new ResponseStatusException(HttpStatus.NOT_FOUND, "Task not found");
        return status;
    }

    @RabbitListener(queues = "word-results")
    public void receive(String result) {
        String[] parts = result.split("\t", 3);
        if (parts.length != 3) throw new IllegalArgumentException("Invalid result: " + result);
        int wordCount = Integer.parseInt(parts[1]);
        tasks.computeIfPresent(parts[0], (id, previous) -> new TaskStatus("done", wordCount, parts[2]));
    }

    public record TaskStatus(String status, Integer wordCount, String worker) {}
}
