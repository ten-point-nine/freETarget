import 'dart:math';
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
          pow(shots[i].x - shots[j].x, 2) + 
          pow(shots[i].y - shots[j].y, 2)
        );
        if (distance > maxDistance) {
          maxDistance = distance;
        }
      }
    }
    
    return maxDistance;
  }

  /// Calculate Mean Radius (MR) - average distance of shots from MPI
  static double calculateMeanRadius(List<Shot> shots) {
    if (shots.isEmpty) return 0;
    
    final mpi = calculateMPI(shots);
    double totalDistance = 0;
    
    for (var shot in shots) {
      totalDistance += sqrt(
        pow(shot.x - mpi.x, 2) + 
        pow(shot.y - mpi.y, 2)
      );
    }
    
    return totalDistance / shots.length;
  }

  /// Calculate Horizontal and Vertical Spread
  static Map<String, double> calculateGroupSpread(List<Shot> shots) {
    if (shots.isEmpty) {
      return {'horizontal': 0, 'vertical': 0};
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
    
    return {
      'horizontal': (maxX - minX).abs(),
      'vertical': (maxY - minY).abs(),
    };
  }
}