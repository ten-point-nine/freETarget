import 'package:flutter/material.dart';
import '../models/shot.dart';

class AirPistolTargetPainter extends CustomPainter {
  final List<Shot> shots;
  final double zoomLevel;

  AirPistolTargetPainter(this.shots, this.zoomLevel);

  static const double baseScale = 1.25;

  static const List<double> ringDiameters = [
    170.0, // Target card size
    155.5, // Ring 1 (outermost)
    139.5, // Ring 2
    123.5, // Ring 3
    107.5, // Ring 4
    91.5,  // Ring 5
    75.5,  // Ring 6
    59.5,  // Ring 7 (start of black area)
    43.5,  // Ring 8
    27.5,  // Ring 9
    11.5,  // Ring 10
    5.0,   // Inner ten
  ];

  @override
  void paint(Canvas canvas, Size size) {
    final scale = size.width / (ringDiameters[0] * 1.2);
    final effectiveScale = scale * baseScale * zoomLevel;
    final center = Offset(size.width / 2, size.height / 2);

    // Draw white background
    canvas.drawRect(
      Rect.fromLTWH(0, 0, size.width, size.height),
      Paint()..color = Colors.white,
    );

    // Draw black background (7-10 rings)
    canvas.drawCircle(
      center,
      (ringDiameters[7] / 2) * effectiveScale,
      Paint()..color = Colors.black,
    );

    // Draw rings
    final ringPaint = Paint()
      ..style = PaintingStyle.stroke
      ..strokeWidth = 0.2 * effectiveScale;

    // Draw from outside in
    for (int i = 1; i < ringDiameters.length; i++) {
      final diameter = ringDiameters[i];
      final isInBlackArea = diameter <= 59.5;
      ringPaint.color = isInBlackArea ? Colors.white : Colors.black;

      canvas.drawCircle(
        center,
        (diameter / 2) * effectiveScale,
        ringPaint,
      );

      // Draw numbers 1-8 between rings
      if (i < 9) {
        final outerRadius = ringDiameters[i] / 2;
        final innerRadius = ringDiameters[i + 1] / 2;
        final midRadius = (outerRadius + innerRadius) / 2;
        final ringNumber = i;
        final fontSize = (outerRadius - innerRadius) * 0.5 * effectiveScale;

        final textPainter = TextPainter(
          text: TextSpan(
            text: ringNumber.toString(),
            style: TextStyle(
              color: ringNumber >= 7 ? Colors.white : Colors.black,
              fontSize: fontSize,
              fontWeight: FontWeight.normal,
            ),
          ),
          textDirection: TextDirection.ltr,
        );

        textPainter.layout();

        // Draw numbers in + pattern
        final positions = [
          (0.0, -1.0), // Top
          (1.0, 0.0),  // Right
          (0.0, 1.0),  // Bottom
          (-1.0, 0.0), // Left
        ];

        for (var (dx, dy) in positions) {
          final x = center.dx + midRadius * effectiveScale * dx - textPainter.width / 2;
          final y = center.dy + midRadius * effectiveScale * dy - textPainter.height / 2;
          textPainter.paint(canvas, Offset(x, y));
        }
      }
    }

    // Draw shots
    for (int i = 0; i < shots.length; i++) {
      final shot = shots[i];
      final isLatestShot = i == shots.length - 1;

      // ISSF caliber size (4.5mm) scaled to target
      final shotRadius = 2.25 * effectiveScale; // 4.5mm diameter / 2

      // Draw shot hole with gradient (darker rim)
      final Gradient gradient = RadialGradient(
        center: Alignment.center,
        radius: 0.8,
        colors: [
          isLatestShot ? Colors.blue[100]! : Colors.blue[700]!,
          Colors.black,
        ],
      );

      final shotPaint = Paint()
        ..shader = gradient.createShader(
          Rect.fromCircle(
            center: Offset(
              center.dx + (shot.x * effectiveScale),
              center.dy + (shot.y * effectiveScale),
            ),
            radius: shotRadius,
          ),
        );

      // Draw the shot hole
      canvas.drawCircle(
        Offset(
          center.dx + (shot.x * effectiveScale),
          center.dy + (shot.y * effectiveScale),
        ),
        shotRadius,
        shotPaint,
      );

      // Draw shot number inside caliber
      final textPainter = TextPainter(
        text: TextSpan(
          text: (i + 1).toString(),
          style: TextStyle(
            color: Colors.white,
            fontSize: shotRadius * 1.2,
            fontWeight: FontWeight.bold,
          ),
        ),
        textDirection: TextDirection.ltr,
      );
      textPainter.layout();

      textPainter.paint(
        canvas,
        Offset(
          center.dx + (shot.x * effectiveScale) - textPainter.width / 2,
          center.dy + (shot.y * effectiveScale) - textPainter.height / 2,
        ),
      );
    }
  }

  @override
  bool shouldRepaint(covariant CustomPainter oldDelegate) => true;
}

class AirRifleTargetPainter extends CustomPainter {
  final List<Shot> shots;
  final double zoomLevel;

  AirRifleTargetPainter(this.shots, this.zoomLevel);

  static const double baseScale = 2;

  static const List<double> ringDiameters = [
    80.0,  // Target card size
    45.5,  // Ring 1 (outermost)
    40.5,  // Ring 2
    35.5,  // Ring 3
    30.5,  // Ring 4 (start of black area)
    25.5,  // Ring 5
    20.5,  // Ring 6
    15.5,  // Ring 7
    10.5,  // Ring 8
    5.5,   // Ring 9
    0.5,   // Ring 10 (white dot)
  ];

  @override
  void paint(Canvas canvas, Size size) {
    final scale = size.width / (ringDiameters[0] * 1.2);
    final effectiveScale = scale * baseScale * zoomLevel;
    final center = Offset(size.width / 2, size.height / 2);

    // Draw white background
    canvas.drawRect(
      Rect.fromLTWH(0, 0, size.width, size.height),
      Paint()..color = Colors.white,
    );

    // Draw black background (4-9 rings)
    canvas.drawCircle(
      center,
      (ringDiameters[4] / 2) * effectiveScale,
      Paint()..color = Colors.black,
    );

    // Draw rings from outside in
    final ringPaint = Paint()
      ..style = PaintingStyle.stroke
      ..strokeWidth = 0.2 * effectiveScale;

    // Draw all rings (1-9)
    for (int i = 1; i < ringDiameters.length - 1; i++) {
      final diameter = ringDiameters[i];
      final isInBlackArea = diameter <= 30.5; // Ring 4 and inward are black
      ringPaint.color = isInBlackArea ? Colors.white : Colors.black;

      canvas.drawCircle(
        center,
        (diameter / 2) * effectiveScale,
        ringPaint,
      );

      // Draw numbers between rings (1-8)
      if (i < 9) {
        final currentRingRadius = ringDiameters[i] / 2;
        final nextRingRadius = ringDiameters[i + 1] / 2;
        final midRadius = (currentRingRadius + nextRingRadius) / 2;
        final fontSize = (currentRingRadius - nextRingRadius) * 0.6 * effectiveScale;

        final textPainter = TextPainter(
          text: TextSpan(
            text: i.toString(),
            style: TextStyle(
              color: i >= 4 ? Colors.white : Colors.black,
              fontSize: fontSize,
              fontWeight: FontWeight.normal,
            ),
          ),
          textDirection: TextDirection.ltr,
        );

        textPainter.layout();

        // Draw numbers in + pattern
        final positions = [
          (0.0, -1.0), // Top
          (1.0, 0.0),  // Right
          (0.0, 1.0),  // Bottom
          (-1.0, 0.0), // Left
        ];

        for (var (dx, dy) in positions) {
          final x = center.dx + midRadius * effectiveScale * dx - textPainter.width / 2;
          final y = center.dy + midRadius * effectiveScale * dy - textPainter.height / 2;
          textPainter.paint(canvas, Offset(x, y));
        }
      }
    }

    // Draw 10-ring as solid white dot
    canvas.drawCircle(
      center,
      (ringDiameters.last / 2) * effectiveScale,
      Paint()
        ..color = Colors.white
        ..style = PaintingStyle.fill,
    );

    // Draw shots
    for (int i = 0; i < shots.length; i++) {
      final shot = shots[i];
      final isLatestShot = i == shots.length - 1;
      final shotRadius = 2.25 * effectiveScale;

      final Gradient gradient = RadialGradient(
        center: Alignment.center,
        radius: 0.8,
        colors: [
          isLatestShot ? Colors.blue[100]! : Colors.blue[700]!,
          Colors.black,
        ],
      );

      final shotPaint = Paint()
        ..shader = gradient.createShader(
          Rect.fromCircle(
            center: Offset(
              center.dx + (shot.x * effectiveScale),
              center.dy + (shot.y * effectiveScale),
            ),
            radius: shotRadius,
          ),
        );

      canvas.drawCircle(
        Offset(
          center.dx + (shot.x * effectiveScale),
          center.dy + (shot.y * effectiveScale),
        ),
        shotRadius,
        shotPaint,
      );

      final textPainter = TextPainter(
        text: TextSpan(
          text: (i + 1).toString(),
          style: TextStyle(
            color: Colors.white,
            fontSize: shotRadius * 1.2,
            fontWeight: FontWeight.bold,
          ),
        ),
        textDirection: TextDirection.ltr,
      );
      textPainter.layout();

      textPainter.paint(
        canvas,
        Offset(
          center.dx + (shot.x * effectiveScale) - textPainter.width / 2,
          center.dy + (shot.y * effectiveScale) - textPainter.height / 2,
        ),
      );
    }
  }

  @override
  bool shouldRepaint(covariant CustomPainter oldDelegate) => true;
}