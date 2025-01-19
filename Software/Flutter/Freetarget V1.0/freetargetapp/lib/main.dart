import 'package:flutter/material.dart';
import 'screens/home_screen.dart';

void main() {
  runApp(const FreeTargetApp());
}

class FreeTargetApp extends StatelessWidget {
  const FreeTargetApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'FreeTarget 10.9',
      debugShowCheckedModeBanner: false,
      theme: ThemeData(
        primarySwatch: Colors.blue,
        useMaterial3: true,
      ),
      home: const HomeScreen(),
    );
  }
}