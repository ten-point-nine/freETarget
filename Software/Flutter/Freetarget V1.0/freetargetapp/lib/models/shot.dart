import 'dart:math';

enum TargetType {
  airPistol,
  airRifle,
}

class Shot {
  final double x;
  final double y;
  final double score;
  final double decimalScore;
  final bool isInnerTen;
  final DateTime timestamp;

  Shot(this.x, this.y, this.score, this.decimalScore, this.isInnerTen,
      this.timestamp);

  factory Shot.fromJson(Map<String, dynamic> json, TargetType targetType) {
    final double x = (json['x'] as num).toDouble();
    final double y = (json['y'] as num).toDouble();

    final distance = sqrt(pow(x, 2) + pow(y, 2));
    double score = 0.0;
    double decimalScore = 0.0;
    bool isInnerTen = false;

    if (targetType == TargetType.airPistol) {
      if (distance <= 5.0) {
        isInnerTen = true;
      }
      double get10Radius = 11.5 / 2 + 4.5 / 2;
      decimalScore = 11 - (distance / get10Radius);
    } else {
      if (distance <= 0.5) {
        isInnerTen = true;
      }
      double get10Radius = 0.5 / 2 + 4.5 / 2;
      decimalScore = 11 - (distance / get10Radius);
    }

    score = decimalScore.floorToDouble();
    decimalScore = ((decimalScore * 10).floorToDouble()) / 10;

    return Shot(x, -y, score, decimalScore, isInnerTen, DateTime.now());
  }
}