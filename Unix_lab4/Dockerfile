FROM maven:3.9-eclipse-temurin-21 AS build
WORKDIR /app
COPY pom.xml .
COPY api/pom.xml api/pom.xml
COPY worker/pom.xml worker/pom.xml
COPY api/src api/src
COPY worker/src worker/src
ARG MODULE
RUN mvn -q -pl "${MODULE}" -am -DskipTests package && cp "${MODULE}/target/${MODULE}-1.0.0.jar" /app.jar

FROM eclipse-temurin:21-jre
COPY --from=build /app.jar /app.jar
ENTRYPOINT ["java", "-jar", "/app.jar"]
