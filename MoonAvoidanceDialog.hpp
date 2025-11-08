#ifndef MOONAVOIDANCEDIALOG_HPP
#define MOONAVOIDANCEDIALOG_HPP

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QColorDialog>
#include "MoonAvoidanceConfig.hpp"

class MoonAvoidanceDialog : public QDialog
{
	Q_OBJECT

public:
	explicit MoonAvoidanceDialog(QWidget* parent = nullptr);
	~MoonAvoidanceDialog();
	
	void setFilters(const QList<FilterConfig>& filters);
	QList<FilterConfig> getFilters() const;

private slots:
	void addFilter();
	void removeFilter();
	void colorButtonClicked();
	void updateFilterFromRow(int row);
	bool validateInput();

private:
	void setupUI();
	void populateTable(const QList<FilterConfig>& filters);
	QColor getColorFromRow(int row) const;
	void setColorForRow(int row, const QColor& color);
	
	QTableWidget* tableWidget;
	QPushButton* addButton;
	QPushButton* removeButton;
	QPushButton* okButton;
	QPushButton* cancelButton;
	
	QList<FilterConfig> currentFilters;
};

#endif // MOONAVOIDANCEDIALOG_HPP

