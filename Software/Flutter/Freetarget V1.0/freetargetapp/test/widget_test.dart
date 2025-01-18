import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:freetargetapp/main.dart';

void main() {
  testWidgets('App smoke test', (WidgetTester tester) async {
    // Build our app and trigger a frame.
    await tester.pumpWidget(const FreeTargetApp());

    // Verify that our app has a title
    expect(find.byType(MaterialApp), findsOneWidget);

    // Add more widget tests here as needed
  });
}
