import 'dart:math';
import 'package:flutter/material.dart';
import '../models/shot.dart';

class GroupAnalysis {
  /// Calculate Mean Point of Impact (MPI) - the center of the shot group
  static Point<double> calculateMPI(List<Shot> shots) {
    if (shots.isEmpty) return const Point(0, 0);

    double sumX = 0;
    double sumY = 0;

    for (var shot in shots) {
      sumX += shot.x;
      sumY += shot.y;
    }

    return Point(
      sumX / shots.length,
      sumY / shots.length,
    );
  }

  /// Calculate Extreme Spread (ES) - the distance between the two farthest shots
  static double calculateExtremeSpread(List<Shot> shots) {
    if (shots.length < 2) return 0;

    double maxDistance = 0;

    for (int i = 0; i < shots.length; i++) {
      for (int j = i + 1; j < shots.length; j++) {
        double distance = sqrt(
            pow(shots[i].x - shots[j].x, 2) + pow(shots[i].y - shots[j].y, 2));
        if (distance > maxDistance) {
          maxDistance = distance;
        }
      }
    }

    return maxDistance;
  }

  /// Calculate Figure of Merit (FoM) - mean radius from MPI
  static double calculateFigureOfMerit(List<Shot> shots) {
    if (shots.isEmpty) return 0;

    final mpi = calculateMPI(shots);
    double totalDistance = 0;

    for (var shot in shots) {
      totalDistance += sqrt(pow(shot.x - mpi.x, 2) + pow(shot.y - mpi.y, 2));
    }

    return totalDistance / shots.length;
  }

  /// Calculate Diagonal - smallest square containing all shots
  static Map<String, double> calculateDiagonal(List<Shot> shots) {
    if (shots.isEmpty) {
      return {
        'diagonal': 0,
        'width': 0,
        'height': 0,
      };
    }

    double minX = shots[0].x;
    double maxX = shots[0].x;
    double minY = shots[0].y;
    double maxY = shots[0].y;

    for (var shot in shots) {
      minX = min(minX, shot.x);
      maxX = max(maxX, shot.x);
      minY = min(minY, shot.y);
      maxY = max(maxY, shot.y);
    }

    double width = (maxX - minX).abs();
    double height = (maxY - minY).abs();

    return {
      'diagonal': sqrt(pow(width, 2) + pow(height, 2)),
      'width': width,
      'height': height,
    };
  }

  /// Calculate Standard Deviation
  static Map<String, double> calculateStandardDeviation(List<Shot> shots) {
    if (shots.length < 2) {
      return {'x': 0, 'y': 0};
    }

    final mpi = calculateMPI(shots);
    double sumSquaredX = 0;
    double sumSquaredY = 0;

    for (var shot in shots) {
      sumSquaredX += pow(shot.x - mpi.x, 2);
      sumSquaredY += pow(shot.y - mpi.y, 2);
    }

    return {
      'x': sqrt(sumSquaredX / (shots.length - 1)),
      'y': sqrt(sumSquaredY / (shots.length - 1)),
    };
  }

  /// Calculate Shot Distribution by Quadrant
  static Map<String, int> calculateQuadrantDistribution(List<Shot> shots) {
    if (shots.isEmpty) {
      return {
        'topRight': 0,
        'topLeft': 0,
        'bottomRight': 0,
        'bottomLeft': 0,
        'center': 0,
      };
    }

    final mpi = calculateMPI(shots);
    int topRight = 0, topLeft = 0, bottomRight = 0, bottomLeft = 0, center = 0;
    const centerThreshold = 2.0; // mm from MPI to be considered center

    for (var shot in shots) {
      double distanceFromMPI =
          sqrt(pow(shot.x - mpi.x, 2) + pow(shot.y - mpi.y, 2));

      if (distanceFromMPI <= centerThreshold) {
        center++;
      } else {
        if (shot.x >= mpi.x) {
          if (shot.y >= mpi.y) {
            topRight++;
          } else {
            bottomRight++;
          }
        } else {
          if (shot.y >= mpi.y) {
            topLeft++;
          } else {
            bottomLeft++;
          }
        }
      }
    }

    return {
      'topRight': topRight,
      'topLeft': topLeft,
      'bottomRight': bottomRight,
      'bottomLeft': bottomLeft,
      'center': center,
    };
  }
}

class GroupAnalysisPainter extends CustomPainter {
  final List<Shot> shots;
  final double scale;

  GroupAnalysisPainter({
    required this.shots,
    required this.scale,
  });

  @override
  void paint(Canvas canvas, Size size) {
    if (shots.isEmpty) return;

    final center = Offset(size.width / 2, size.height / 2);

    // Calculate MPI
    final mpi = GroupAnalysis.calculateMPI(shots);
    final fom = GroupAnalysis.calculateFigureOfMerit(shots);
    final diagonal = GroupAnalysis.calculateDiagonal(shots);

    // Draw MPI point
    final mpiPaint = Paint()
      ..color = Colors.red
      ..style = PaintingStyle.fill;

    canvas.drawCircle(
      Offset(
        center.dx + (mpi.x * scale),
        center.dy + (mpi.y * scale),
      ),
      3,
      mpiPaint,
    );

    // Draw Figure of Merit circle
    final fomPaint = Paint()
      ..color = Colors.green.withAlpha(128)
      ..style = PaintingStyle.stroke
      ..strokeWidth = 1;

    canvas.drawCircle(
      Offset(
        center.dx + (mpi.x * scale),
        center.dy + (mpi.y * scale),
      ),
      fom * scale,
      fomPaint,
    );

    // Draw bounding box
    final boxPaint = Paint()
      ..color = Colors.blue.withAlpha(77)
      ..style = PaintingStyle.stroke
      ..strokeWidth = 1;

    final rect = Rect.fromCenter(
      center: Offset(
        center.dx + (mpi.x * scale),
        center.dy + (mpi.y * scale),
      ),
      width: diagonal['width']! * scale,
      height: diagonal['height']! * scale,
    );

    canvas.drawRect(rect, boxPaint);

    // Draw measurements
    _drawLabel(
      canvas,
      'FoM: ${fom.toStringAsFixed(1)}mm',
      Offset(10, size.height - 60),
      Colors.green,
    );

    _drawLabel(
      canvas,
      'ES: ${GroupAnalysis.calculateExtremeSpread(shots).toStringAsFixed(1)}mm',
      Offset(10, size.height - 40),
      Colors.red,
    );

    _drawLabel(
      canvas,
      'H: ${diagonal['width']?.toStringAsFixed(1)}mm',
      Offset(10, size.height - 20),
      Colors.blue,
    );

    _drawLabel(
      canvas,
      'V: ${diagonal['height']?.toStringAsFixed(1)}mm',
      Offset(100, size.height - 20),
      Colors.blue,
    );
  }

  void _drawLabel(Canvas canvas, String text, Offset position, Color color) {
    final textPainter = TextPainter(
      text: TextSpan(
        text: text,
        style: TextStyle(
          color: color,
          fontSize: 12,
          fontWeight: FontWeight.bold,
        ),
      ),
      textDirection: TextDirection.ltr,
    );

    textPainter.layout();
    textPainter.paint(canvas, position);
  }

  @override
  bool shouldRepaint(covariant CustomPainter oldDelegate) => true;
}
