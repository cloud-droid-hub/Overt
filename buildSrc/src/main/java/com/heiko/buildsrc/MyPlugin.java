package com.heiko.buildsrc;

import org.gradle.api.Plugin;
import org.gradle.api.Project;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.nio.file.StandardCopyOption;
import java.security.KeyStore;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.security.cert.Certificate;
import java.io.*;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Properties;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class MyPlugin implements Plugin<Project> {
    @Override
    public void apply(Project project) {
        System.out.println("-----------lxz apply----------");

        try {
            // 获取 build.gradle 文件
            File buildFile = project.getBuildFile();
            if (buildFile == null || !buildFile.exists()) {
                System.out.println("build.gradle file not found");
                return;
            }

            // 读取文件内容
            String content = new String(Files.readAllBytes(buildFile.toPath()), "UTF-8");
            System.out.println("Original content read from build.gradle");

            String modifiedContent = content;

            // 匹配 versionCode，格式：versionCode 数字
            Pattern versionCodePattern = Pattern.compile("(versionCode\\s+)(\\d+)");
            Matcher versionCodeMatcher = versionCodePattern.matcher(modifiedContent);
            if (versionCodeMatcher.find()) {
                int currentVersionCode = Integer.parseInt(versionCodeMatcher.group(2));
                int newVersionCode = currentVersionCode + 1;
                modifiedContent = versionCodeMatcher.replaceAll("$1" + newVersionCode);
                System.out.println("Updated versionCode from " + currentVersionCode + " to " + newVersionCode);
            }

            // 匹配 versionName，格式：versionName "x.y" 或 versionName "x"
            Pattern versionNamePattern = Pattern.compile("(versionName\\s+\")(\\d+)(\\.(\\d+))?(\")");
            Matcher versionNameMatcher = versionNamePattern.matcher(modifiedContent);
            if (versionNameMatcher.find()) {
                String prefix = versionNameMatcher.group(1);
                int majorVersion = Integer.parseInt(versionNameMatcher.group(2));
                String minorPart = versionNameMatcher.group(3); // 包含点和小版本号，如 ".0"
                String suffix = versionNameMatcher.group(5);
                
                String newVersionName;
                if (minorPart != null) {
                    // 有小数部分，小版本号 +1
                    int minorVersion = Integer.parseInt(versionNameMatcher.group(4));
                    int newMinorVersion = minorVersion + 1;
                    newVersionName = prefix + majorVersion + "." + newMinorVersion + suffix;
                    System.out.println("Updated versionName from \"" + majorVersion + "." + minorVersion + 
                                     "\" to \"" + majorVersion + "." + newMinorVersion + "\"");
                } else {
                    // 没有小数部分，主版本号 +1
                    int newMajorVersion = majorVersion + 1;
                    newVersionName = prefix + newMajorVersion + suffix;
                    System.out.println("Updated versionName from \"" + majorVersion + 
                                     "\" to \"" + newMajorVersion + "\"");
                }
                modifiedContent = versionNameMatcher.replaceAll(Matcher.quoteReplacement(newVersionName));
            }
            
            // 如果内容有变化，写回文件
            if (!content.equals(modifiedContent)) {
                Files.write(buildFile.toPath(), modifiedContent.getBytes("UTF-8"));
                System.out.println("Successfully updated build.gradle");
            } else {
                System.out.println("No changes made");
            }
        } catch (IOException e) {
            System.err.println("Error reading/writing build.gradle: " + e.getMessage());
            e.printStackTrace();
        } catch (NumberFormatException e) {
            System.err.println("Error parsing version number: " + e.getMessage());
            e.printStackTrace();
        }
    }
}
