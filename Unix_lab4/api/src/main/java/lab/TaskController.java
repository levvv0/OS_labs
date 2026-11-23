package main;

import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;
import org.springframework.web.server.ResponseStatusException;

@RestController
@RequestMapping("/tasks")
public class TaskController {
    private final TaskService service;

    public TaskController(TaskService service) {
        this.service = service;
    }

    @PostMapping
    public ResponseEntity<TaskCreated> create(@RequestBody TaskRequest request) {
        if (request.text() == null || request.text().isBlank()) {
            throw new ResponseStatusException(HttpStatus.BAD_REQUEST, "Text is required");
        }
        String id = service.submit(request.text());
        return ResponseEntity.accepted().body(new TaskCreated(id, "queued"));
    }

    @GetMapping("/{id}")
    public TaskService.TaskStatus get(@PathVariable String id) {
        return service.get(id);
    }

    public record TaskRequest(String text) {}
    public record TaskCreated(String id, String status) {}
}
