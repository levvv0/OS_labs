package main;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.amqp.rabbit.annotation.RabbitListener;
import org.springframework.amqp.rabbit.core.RabbitTemplate;
import org.springframework.stereotype.Component;

@Component
public class WordWorker {
    private static final Logger log = LoggerFactory.getLogger(WordWorker.class);
    private final RabbitTemplate rabbitTemplate;
    private final String workerId = System.getenv().getOrDefault("HOSTNAME", "local-worker");

    public WordWorker(RabbitTemplate rabbitTemplate) {
        this.rabbitTemplate = rabbitTemplate;
    }

    @RabbitListener(queues = "word-tasks")
    public void process(String task) throws InterruptedException {
        String[] parts = task.split("\t", 2);
        if (parts.length != 2) throw new IllegalArgumentException("Invalid task");

        Thread.sleep(1000);
        int wordCount = parts[1].trim().split("\\s+").length;
        rabbitTemplate.convertAndSend("word-results", parts[0] + "\t" + wordCount + "\t" + workerId);
        log.info("Task {} completed by {}: {} words", parts[0], workerId, wordCount);
    }
}
